// OCP visualisation helper: ring of chip snapshots (see gme_vis_enable)
#ifndef GME_VIS_H
#define GME_VIS_H

#include "gme.h"
#include <string.h>
#include <new>

struct Gme_Vis
{
	enum { ring_size = 1024 };
	bool on;
	bool have;
	gme_vis_frame_t cur;     // frame being built (state is filled when it is pushed)
	long cur_sample;
	double clock_base;       // clocks of previous run_clocks() blocks
	double next_clock;       // next snapshot, absolute clocks
	gme_vis_frame_t* ring;
	int head;
	int count;

	Gme_Vis() : on( false ), ring( 0 ), head( 0 ), count( 0 ) { reset(); }
	~Gme_Vis() { delete [] ring; }

	void reset()
	{
		memset( &cur, 0, sizeof cur );
		have = false;
		cur_sample = 0;
		clock_base = 0;
		next_clock = 0;
		head = 0;
		count = 0;
	}

	bool enable( bool e )
	{
		if ( e && !ring )
		{
			ring = new (std::nothrow) gme_vis_frame_t [ring_size];
			if ( !ring )
				return false;
		}
		on = e;
		head = 0;
		count = 0;
		return true;
	}

	int read( gme_vis_frame_t* out, int max )
	{
		int n = 0;
		while ( n < max && count )
		{
			int tail = (head - count + ring_size) % ring_size;
			out [n++] = ring [tail];
			count--;
		}
		return n;
	}

	// true if a snapshot is due at absolute clock 'abs' (at most 'rate' per second)
	bool due( double abs, double clock_rate, double rate = 60.0 )
	{
		if ( abs < next_clock )
			return false;
		double period = clock_rate / rate;
		next_clock += period;
		if ( next_clock <= abs )
			next_clock = abs + period * 0.5; // late, resynchronize
		return true;
	}

	void push()
	{
		if ( !ring )
			return;
		cur.sample = cur_sample;
		ring [head] = cur;
		head = (head + 1) % ring_size;
		if ( count < ring_size )
			count++;
	}
};

#endif
