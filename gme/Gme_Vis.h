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
	bool light;              // registers only: the emulator may skip sound synthesis (output is not usable)
	bool have;
	gme_vis_frame_t cur;     // frame being built (state is filled when it is pushed)
	long cur_sample;
	double clock_base;       // clocks of previous run_clocks() blocks
	double next_clock;       // next snapshot, absolute clocks
	gme_vis_frame_t* ring;
	int head;
	int count;

	Gme_Vis() : on( false ), light( false ), ring( 0 ), head( 0 ), count( 0 ) { reset(); }
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
		if ( !e )
			light = false;
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

// shadow registers of the Genesis / Master System chips (VGM, GYM)
struct Gme_Vis_Genesis
{
	int psg_ch, psg_type; // SN76489 latch

	void reset() { psg_ch = 0; psg_type = 0; }

	static void ym2612( gme_vis_frame_t& f, int port, int addr, int data )
	{
		addr &= 0xFF;
		f.ym2612 [port & 1][addr] = (unsigned char) data;
		if ( port == 0 && addr == 0x28 )
		{
			int c = data & 7;
			if ( (c & 3) == 3 )
				return;
			c = (c & 3) + ((c & 4) ? 3 : 0);
			int key = (data >> 4) & 0x0F;
			if ( key && !f.ym2612_key [c] )
				f.ym2612_trig |= 1 << c;
			f.ym2612_key [c] = (unsigned char) key;
		}
		else if ( port == 0 && addr == 0x2B )
			f.ym2612_dac = (data >> 7) & 1;
	}

	static void dac( gme_vis_frame_t& f )
	{
		if ( f.ym2612_dac_writes < 0xFFFF )
			f.ym2612_dac_writes++;
	}

	static void psg_vol( gme_vis_frame_t& f, int ch, int v )
	{
		int old = f.psg_vol [ch];
		if ( v < 15 && (old == 15 || old - v >= 4) )
			f.psg_trig |= 1 << ch;
		f.psg_vol [ch] = (unsigned char) v;
	}

	void psg( gme_vis_frame_t& f, int data )
	{
		if ( data & 0x80 )
		{
			psg_ch = (data >> 5) & 3;
			psg_type = (data >> 4) & 1;
			if ( psg_type )
				psg_vol( f, psg_ch, data & 0x0F );
			else if ( psg_ch < 3 )
				f.psg_period [psg_ch] = (unsigned short) ((f.psg_period [psg_ch] & 0x3F0) | (data & 0x0F));
			else
				f.psg_noise = (unsigned char) (data & 7);
		}
		else if ( psg_type )
			psg_vol( f, psg_ch, data & 0x0F );
		else if ( psg_ch < 3 )
			f.psg_period [psg_ch] = (unsigned short) ((f.psg_period [psg_ch] & 0x0F) | ((data & 0x3F) << 4));
		else
			f.psg_noise = (unsigned char) (data & 7);
	}

	static void ym2413( gme_vis_frame_t& f, int addr, int data )
	{
		addr &= 0x3F;
		if ( addr >= 0x20 && addr <= 0x28 && (data & 0x10) && !(f.ym2413 [addr] & 0x10) )
			f.ym2413_trig |= 1 << (addr - 0x20);
		f.ym2413 [addr] = (unsigned char) data;
	}

	// the frame keeps the registers; clear the per-frame events after it is pushed
	static void next_frame( gme_vis_frame_t& f )
	{
		f.ym2612_trig = 0;
		f.ym2612_dac_writes = 0;
		f.psg_trig = 0;
		f.ym2413_trig = 0;
	}

	static void init_frame( gme_vis_frame_t& f )
	{
		for ( int i = 0; i < 4; i++ )
			f.psg_vol [i] = 15;
		for ( int i = 0; i < 3; i++ ) // YM2612 outputs are enabled on both sides after reset
			f.ym2612 [0][0xB4 + i] = f.ym2612 [1][0xB4 + i] = 0xC0;
	}
};

#endif
