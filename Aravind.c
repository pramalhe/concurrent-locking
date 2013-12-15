// Aravind, J. Parallel Distrib. Comput. 73 (2013) p. 1033, Fig. 3
// Moved turn[id] = 0; after the critical section for performance reasons.

volatile TYPE *intents, *turn;

static void *Worker( void *arg ) {
	unsigned int id = (size_t)arg;
	TYPE copy[N];
	int j, t;
#ifdef FAST
	unsigned int cnt = 0;
#endif // FAST
	size_t entries[RUNS];

	for ( int r = 0; r < RUNS; r += 1 ) {
		entries[r] = 0;
		t = 1;
		while ( stop == 0 ) {
#ifdef FAST
			id = startpoint( cnt );						// different starting point each experiment
			cnt = cycleUp( cnt, NoStartPoints );
#endif // FAST
			intents[id] = 1;							// phase 1, FCFS
			Fence();									// force store before more loads
			for ( j = 0; j < N; j += 1 )				// copy turn values
				copy[j] = turn[j];
			turn[id] = t;								// advance turn
			intents[id] = 0;
			Fence();									// force store before more loads
			for ( j = 0; j < N; j += 1 )
				if ( copy[j] != 0 )						// want in ?
					while ( copy[j] == turn[j] ) Pause();
		  L: intents[id] = 1;							// phase 2, B-L entry protocol, stage 1
			Fence();									// force store before more loads
			for ( j = 0; j < id; j += 1 )
				if ( intents[j] != 0 ) {
					intents[id] = 0;
					Fence();							// force store before more loads
					while ( intents[j] != 0 ) Pause();
					goto L;								// restart
				} // if
			for ( j = id + 1; j < N; j += 1 )			// B-L entry protocol, stage 2
				while ( intents[j] != 0 ) Pause();
//			turn[id] = 0;								// original position
			CriticalSection( id );
			intents[id] = 0;							// B-L exit protocol
			turn[id] = 0;
			t = t < 3 ? t + 1 : 1;						// [1..3]
			entries[r] += 1;
		} // while
		__sync_fetch_and_add( &Arrived, 1 );
		while ( stop != 0 ) Pause();
		__sync_fetch_and_add( &Arrived, -1 );
	} // for
	qsort( entries, RUNS, sizeof(size_t), compare );
	return (void *)median(entries);
} // Worker

void ctor() {
	intents = Allocator( sizeof(volatile TYPE) * N );
	for ( int i = 0; i < N; i += 1 ) {
		intents[i] = 0;
	} // for
	turn = Allocator( sizeof(volatile TYPE) * N );
	for ( int i = 0; i < N; i += 1 ) {
		turn[i] = 0;
	} // for
} // ctor

void dtor() {
	free( (void *)turn );
	free( (void *)intents );
} // dtor

// Local Variables: //
// tab-width: 4 //
// compile-command: "gcc -Wall -std=gnu99 -O3 -DAlgorithm=Aravind Harness.c -lpthread -lm" //
// End: //