// Eric C. R. Hehner and R. K. Shyamasundar, An Implementation of P and V, Information Processing Letters, 1981, 12(4),
// pp. 196-197

enum { MAX_TICKET = INTPTR_MAX };

static volatile TYPE *ticket CALIGN;

static void *Worker( void *arg ) {
	TYPE id = (size_t)arg;
	uint64_t entry;
#ifdef FAST
	unsigned int cnt = 0, oid = id;
#endif // FAST

	for ( int r = 0; r < RUNS; r += 1 ) {
		entry = 0;
		while ( stop == 0 ) {
			// step 1, select a ticket
			ticket[id] = 0;								// set highest priority
			Fence();									// force store before more loads
			TYPE max = 0;								// O(N) search for largest ticket
			for ( int j = 0; j < N; j += 1 ) {
				TYPE v = ticket[j];						// could change so copy
				if ( max < v && v != MAX_TICKET ) max = v;
			} // for
#if 1
			max += 1;									// advance ticket
			ticket[id] = max;
			Fence();									// force store before more loads
			// step 2, wait for ticket to be selected
			for ( int j = 0; j < N; j += 1 )			// check other tickets
				while ( ticket[j] < max ||				// busy wait if choosing or
						( ticket[j] == max && j < id ) ) Pause(); //  greater ticket value or lower priority
#else
			ticket[id] = max + 1;
			Fence();									// force store before more loads
			// step 2, wait for ticket to be selected
			for ( int j = 0; j < N; j += 1 )			// check other tickets
				while ( ticket[j] < ticket[id] ||		// busy wait if choosing or
						( ticket[j] == ticket[id] && j < id ) ) Pause(); //  greater ticket value or lower priority
#endif
			CriticalSection( id );
			ticket[id] = MAX_TICKET;					// exit protocol
#ifdef FAST
			id = startpoint( cnt );						// different starting point each experiment
			cnt = cycleUp( cnt, NoStartPoints );
#endif // FAST
			entry += 1;
		} // while
#ifdef FAST
		id = oid;
#endif // FAST
		entries[r][id] = entry;
		__sync_fetch_and_add( &Arrived, 1 );
		while ( stop != 0 ) Pause();
		__sync_fetch_and_add( &Arrived, -1 );
	} // for
	return NULL;
} // Worker

void ctor() {
	ticket = Allocator( sizeof(typeof(ticket[0])) * N );
	for ( int i = 0; i < N; i += 1 ) {					// initialize shared data
		ticket[i] = MAX_TICKET;
	} // for
} // ctor

void dtor() {
	free( (void *)ticket );
} // dtor

// Local Variables: //
// tab-width: 4 //
// compile-command: "gcc -Wall -std=gnu99 -O3 -DNDEBUG -fno-reorder-functions -DPIN -DAlgorithm=Hehner Harness.c -lpthread -lm" //
// End: //
