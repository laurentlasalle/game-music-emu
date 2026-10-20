/* Game music emulator library C interface (also usable from C++) */

/* Game_Music_Emu 0.6.5 */
#ifndef GME_H
#define GME_H

#ifdef __cplusplus
	extern "C" {
#endif

#define GME_VERSION 0x000605 /* 1 byte major, 1 byte minor, 1 byte patch-level */

/* Error string returned by library functions, or NULL if no error (success) */
typedef const char* gme_err_t;

/* First parameter of most gme_ functions is a pointer to the Music_Emu */
typedef struct Music_Emu Music_Emu;


/* Setup compiler defines useful for exporting required public API symbols in gme.cpp */
#ifndef BLARGG_EXPORT
    #if defined (_WIN32)
        #if defined(BLARGG_BUILD_DLL)
            #define BLARGG_EXPORT __declspec(dllexport)
        #else
            #define BLARGG_EXPORT /* Leave blank: friendly with both static and shared linking. */
        #endif
    #elif defined (LIBGME_VISIBILITY)
        #define BLARGG_EXPORT __attribute__((visibility ("default")))
    #else
        #define BLARGG_EXPORT
    #endif
#endif


/******** Basic operations ********/

/* Create emulator and load game music file/data into it. Sets *out to new emulator. */
BLARGG_EXPORT gme_err_t gme_open_file( const char path [], Music_Emu** out, int sample_rate );

/* Number of tracks available */
BLARGG_EXPORT int gme_track_count( Music_Emu const* );

/* Start a track, where 0 is the first track */
BLARGG_EXPORT gme_err_t gme_start_track( Music_Emu*, int index );

/* Generate 'count' 16-bit signed samples info 'out'. Output is in stereo. */
BLARGG_EXPORT gme_err_t gme_play( Music_Emu*, int count, short out [] );

/* Finish using emulator and free memory */
BLARGG_EXPORT void gme_delete( Music_Emu* );


/******** Track position/length ********/

/* Set time to start fading track out. Once fade ends track_ended() returns true.
Fade time can be changed while track is playing. */
BLARGG_EXPORT void gme_set_fade( Music_Emu*, int start_msec );

/** See gme_set_fade.
 * @since 0.6.4
 */
BLARGG_EXPORT void gme_set_fade_msecs( Music_Emu*, int start_msec, int length_msecs );

/**
 * If do_autoload_limit is nonzero, then automatically load track length
 * metadata (if present) and terminate playback once the track length has been
 * reached. Otherwise playback will continue for an arbitrary period of time
 * until a prolonged period of silence is detected.
 *
 * Not all individual emulators support this setting.
 *
 * By default, playback limits are loaded and applied.
 *
 * @since 0.6.3
 */
BLARGG_EXPORT void gme_set_autoload_playback_limit( Music_Emu *, int do_autoload_limit );

/** See gme_set_autoload_playback_limit.
 * (This was actually added in 0.6.3, but wasn't exported because of a typo.)
 * @since 0.6.4
 */
BLARGG_EXPORT int gme_autoload_playback_limit( Music_Emu const* );

/* True if a track has reached its end */
BLARGG_EXPORT int gme_track_ended( Music_Emu const* );

/* Number of milliseconds (1000 = one second) played since beginning of track */
BLARGG_EXPORT int gme_tell( Music_Emu const* );

/* Number of samples generated since beginning of track */
BLARGG_EXPORT int gme_tell_samples( Music_Emu const* );

/* Number of milliseconds played since beginning of track (scaled with tempo).
 * @since 0.6.5 */
BLARGG_EXPORT int gme_tell_scaled( Music_Emu const* );

/* Seek to new time in track. Seeking backwards or far forward can take a while. */
BLARGG_EXPORT gme_err_t gme_seek( Music_Emu*, int msec );

/* Equivalent to restarting track then skipping n samples */
BLARGG_EXPORT gme_err_t gme_seek_samples( Music_Emu*, int n );

/* Seek to new time in track (scaled with tempo).
 * @since 0.6.5 */
BLARGG_EXPORT gme_err_t gme_seek_scaled( Music_Emu*, int msec );


/******** Informational ********/

/* If you only need track information from a music file, pass gme_info_only for
sample_rate to open/load. */
enum { gme_info_only = -1 };

/* Most recent warning string, or NULL if none. Clears current warning after returning.
Warning is also cleared when loading a file and starting a track. */
BLARGG_EXPORT const char* gme_warning( Music_Emu* );

/* Load m3u playlist file (must be done after loading music) */
BLARGG_EXPORT gme_err_t gme_load_m3u( Music_Emu*, const char path [] );

/* Clear any loaded m3u playlist and any internal playlist that the music format
supports (NSFE for example). */
BLARGG_EXPORT void gme_clear_playlist( Music_Emu* );

/* Gets information for a particular track (length, name, author, etc.).
Must be freed after use. */
typedef struct gme_info_t gme_info_t;
BLARGG_EXPORT gme_err_t gme_track_info( Music_Emu const*, gme_info_t** out, int track );

/* Frees track information */
BLARGG_EXPORT void gme_free_info( gme_info_t* );

struct gme_info_t
{
	/* times in milliseconds; -1 if unknown */
	int length;			/* total length, if file specifies it */
	int intro_length;	/* length of song up to looping section */
	int loop_length;	/* length of looping section */

	/* Length if available, otherwise intro_length+loop_length*2 if available,
	otherwise a default of 150000 (2.5 minutes). */
	int play_length;

	/* fade length in milliseconds; -1 if unknown */
	int fade_length;

	int i5,i6,i7,i8,i9,i10,i11,i12,i13,i14,i15; /* reserved */

	/* empty string ("") if not available */
	const char* system;
	const char* game;
	const char* song;
	const char* author;
	const char* copyright;
	const char* comment;
	const char* dumper;

	const char *s7,*s8,*s9,*s10,*s11,*s12,*s13,*s14,*s15; /* reserved */
};


/******** NSF register visualisation (OCP extension) ********/

/* One entry per NSF play-routine call (one video frame), describing the sound
 * chip state produced by that frame. 'sample' is the output sample frame
 * (counted from gme_start_track, as returned by gme_play) where the frame
 * becomes audible. Register arrays hold the last value written by the music. */
typedef struct gme_nsf_frame_t
{
	long          sample;
	unsigned int  chips;          /* NSF header expansion flags: 1=VRC6 2=VRC7 4=FDS 8=MMC5 16=N163 32=Sunsoft 5B */
	int           pal;            /* 1 if the tune runs at PAL speed / clock */
	unsigned char apu [0x18];     /* $4000-$4017 */
	unsigned char apu_vol [5];    /* volume currently output by pulse1, pulse2, triangle (0/15), noise, dmc (dac >> 3) */
	unsigned char apu_active;     /* bit n: 2A03 channel n is audible */
	unsigned char apu_trig;       /* bit n: 2A03 channel n was (re)started during the frame */
	unsigned char vrc6 [3][3];    /* $9000-2, $A000-2, $B000-2 */
	unsigned char fme7 [16];
	unsigned char mmc5 [0x16];    /* $5000-$5015 */
	unsigned char fds  [0x0B];    /* $4080-$408A */
	unsigned char n163 [0x80];
	unsigned char vrc7 [0x40];
	unsigned int  exp_trig;       /* expansion channel restarted during the frame: bit 0..2 VRC6 (pulse1, pulse2, saw),
	                                 bit 3..5 MMC5 (pulse1, pulse2, pcm), bit 6..11 VRC7 key-on, bit 12..19 N163, bit 20..22 5B */

	int           system;         /* 0 = NSF/NSFe (fields above), 1 = HES (hes_*), 2 = GBS (gb_*) */

	/* HES (PC Engine HuC6280 PSG), snapshot of the 6 channels */
	unsigned short hes_period [6];   /* 12 bit frequency divider */
	unsigned char hes_control [6];   /* $0804: bit7 enable, bit6 DDA, bit0-4 volume */
	unsigned char hes_balance [6];   /* $0805: left / right nibbles */
	unsigned char hes_noise [6];     /* $0807 (channels 5 and 6): bit7 noise enable, bit0-4 frequency */
	unsigned char hes_wave [6][32];  /* waveform, 5 bit samples */
	unsigned char hes_main_balance;  /* $0801 */
	unsigned char hes_trig;          /* bit n: channel n enabled or volume raised during the frame */

	/* GBS (Game Boy APU) */
	unsigned char gb_regs [0x30];    /* $FF10-$FF3F, wave RAM included */
	unsigned char gb_vol [4];        /* current envelope volume 0-15 of square1, square2, (wave), noise */
	unsigned char gb_enabled;        /* bit n: channel n running (length counter not expired) */
	unsigned char gb_trig;           /* bit n: channel n triggered during the frame */

	/* VGM / GYM (Sega Genesis / Master System): first chip of each kind (system 3) */
	unsigned char  vgm_chips;          /* 1 = SN76489, 2 = YM2413, 4 = YM2612 */
	long           psg_clock, ym2413_clock, ym2612_clock;
	unsigned char  ym2612 [2][0x100];  /* registers of port 0 / port 1 */
	unsigned char  ym2612_key [6];     /* operators keyed on (bits 4-7 of $28) per channel */
	unsigned char  ym2612_trig;        /* bit n: channel n keyed on during the frame */
	unsigned char  ym2612_dac;         /* DAC enabled ($2B bit 7) */
	unsigned short ym2612_dac_writes;  /* DAC samples written during the frame */
	unsigned short psg_period [3];     /* SN76489 tone dividers */
	unsigned char  psg_noise;          /* noise control (bit 2 white, bits 0-1 rate) */
	unsigned char  psg_vol [4];        /* attenuation, 0 = loudest, 15 = off */
	unsigned char  psg_trig;           /* bit n: channel n started (volume raised) during the frame */
	unsigned char  ym2413 [0x40];
	unsigned short ym2413_trig;        /* bit n: channel n keyed on during the frame */

	/* SPC (SNES S-DSP), system 4 */
	unsigned char  spc_dsp [0x80];     /* DSP registers; ENVX ($x8) is the live envelope */
	unsigned char  spc_kon;            /* voices keyed on during the frame */
} gme_nsf_frame_t;

typedef gme_nsf_frame_t gme_vis_frame_t;

/* Enable/disable collecting frames. Returns 0 if the emulator is not an NSF/NSFe emulator. */
BLARGG_EXPORT int gme_nsf_vis_enable( Music_Emu*, int enable );

/* Pops up to 'max' collected frames, oldest first. Returns the number of frames copied. */
BLARGG_EXPORT int gme_nsf_vis_read( Music_Emu*, gme_nsf_frame_t* out, int max );

/* Same for NSF, NSFe, HES, GBS, VGM, GYM and SPC; for formats without a play routine
 * a frame is taken 60 times per second. */
BLARGG_EXPORT int gme_vis_enable( Music_Emu*, int enable );
BLARGG_EXPORT int gme_vis_read( Music_Emu*, gme_vis_frame_t* out, int max );

/* SPC only: the 64 KB sound RAM (samples are referenced by the DSP registers). NULL otherwise. */
BLARGG_EXPORT const unsigned char* gme_vis_ram( Music_Emu* );


/******** Advanced playback ********/

/* Adjust stereo echo depth, where 0.0 = off and 1.0 = maximum. Has no effect for
GYM, SPC, and Sega Genesis VGM music */
BLARGG_EXPORT void gme_set_stereo_depth( Music_Emu*, double depth );

/* Disable automatic end-of-track detection and skipping of silence at beginning
if ignore is true */
BLARGG_EXPORT void gme_ignore_silence( Music_Emu*, int ignore );

/* Adjust song tempo, where 1.0 = normal, 0.5 = half speed, 2.0 = double speed.
Track length as returned by track_info() assumes a tempo of 1.0. */
BLARGG_EXPORT void gme_set_tempo( Music_Emu*, double tempo );

/* Number of voices used by currently loaded file */
BLARGG_EXPORT int gme_voice_count( Music_Emu const* );

/* Name of voice i, from 0 to gme_voice_count() - 1 */
BLARGG_EXPORT const char* gme_voice_name( Music_Emu const*, int i );

/* Mute/unmute voice i, where voice 0 is first voice */
BLARGG_EXPORT void gme_mute_voice( Music_Emu*, int index, int mute );

/* Set muting state of all voices at once using a bit mask, where -1 mutes all
voices, 0 unmutes them all, 0x01 mutes just the first voice, etc. */
BLARGG_EXPORT void gme_mute_voices( Music_Emu*, int muting_mask );

/* Disable/Enable echo effect for SPC files */
/* Available since 0.6.4 */
BLARGG_EXPORT void gme_disable_echo( Music_Emu*, int disable );

/* Frequency equalizer parameters (see gme.txt) */
/* Implementers: If modified, also adjust Music_Emu::make_equalizer as needed */
typedef struct gme_equalizer_t
{
	double treble; /* -50.0 = muffled, 0 = flat, +5.0 = extra-crisp */
	double bass;   /* 1 = full bass, 90 = average, 16000 = almost no bass */

	double d2,d3,d4,d5,d6,d7,d8,d9; /* reserved */
} gme_equalizer_t;

/* Get current frequency equalizater parameters */
BLARGG_EXPORT void gme_equalizer( Music_Emu const*, gme_equalizer_t* out );

/* Change frequency equalizer parameters */
BLARGG_EXPORT void gme_set_equalizer( Music_Emu*, gme_equalizer_t const* eq );

/* Enables/disables most accurate sound emulation options */
BLARGG_EXPORT void gme_enable_accuracy( Music_Emu*, int enabled );


/******** Game music types ********/

/* Music file type identifier. Can also hold NULL. */
typedef const struct gme_type_t_* gme_type_t;

/* Emulator type constants for each supported file type */
extern BLARGG_EXPORT const gme_type_t
	gme_ay_type,
	gme_gbs_type,
	gme_gym_type,
	gme_hes_type,
	gme_kss_type,
	gme_nsf_type,
	gme_nsfe_type,
	gme_sap_type,
	gme_spc_type,
	gme_vgm_type,
	gme_vgz_type;

/* Type of this emulator */
BLARGG_EXPORT gme_type_t gme_type( Music_Emu const* );

/* Pointer to array of all music types, with NULL entry at end. Allows a player linked
to this library to support new music types without having to be updated. */
BLARGG_EXPORT gme_type_t const* gme_type_list( void );

/* Name of game system for this music file type */
BLARGG_EXPORT const char* gme_type_system( gme_type_t );

/* True if this music file type supports multiple tracks */
BLARGG_EXPORT int gme_type_multitrack( gme_type_t );

/* whether the pcm output retrieved by gme_play() will have all 8 voices rendered to their
 * individual stereo channel or (if false) these voices get mixed into one single stereo channel
 * @since 0.6.3 */
BLARGG_EXPORT int gme_multi_channel( Music_Emu const* );

/******** Advanced file loading ********/

/* Error returned if file type is not supported */
extern BLARGG_EXPORT const char* const gme_wrong_file_type;

/* Same as gme_open_file(), but uses file data already in memory. Makes copy of data.
 * The resulting Music_Emu object will be set to single channel mode. */
BLARGG_EXPORT gme_err_t gme_open_data( void const* data, long size, Music_Emu** out, int sample_rate );

/* Determine likely game music type based on first four bytes of file. Returns
string containing proper file suffix (i.e. "NSF", "SPC", etc.) or "" if
file header is not recognized. */
BLARGG_EXPORT const char* gme_identify_header( void const* header );

/* Get corresponding music type for file path or extension passed in. */
BLARGG_EXPORT gme_type_t gme_identify_extension( const char path_or_extension [] );

/**
 * Get typical file extension for a given music type.  This is not a replacement
 * for a file content identification library (but see gme_identify_header).
 *
 * @since 0.6.3
 */
BLARGG_EXPORT const char* gme_type_extension( gme_type_t music_type );

/* Determine file type based on file's extension or header (if extension isn't recognized).
Sets *type_out to type, or 0 if unrecognized or error. */
BLARGG_EXPORT gme_err_t gme_identify_file( const char path [], gme_type_t* type_out );

/* Create new emulator and set sample rate. Returns NULL if out of memory. If you only need
track information, pass gme_info_only for sample_rate. */
BLARGG_EXPORT Music_Emu* gme_new_emu( gme_type_t, int sample_rate );

/* Create new multichannel emulator and set sample rate. Returns NULL if out of memory.
 * If you only need track information, pass gme_info_only for sample_rate.
 * (see gme_multi_channel for more information on multichannel support)
 * @since 0.6.3
 */
BLARGG_EXPORT Music_Emu* gme_new_emu_multi_channel( gme_type_t, int sample_rate );

/* Load music file into emulator */
BLARGG_EXPORT gme_err_t gme_load_file( Music_Emu*, const char path [] );

/* Load music file from memory into emulator. Makes a copy of data passed. */
BLARGG_EXPORT gme_err_t gme_load_data( Music_Emu*, void const* data, long size );

/* Load multiple single-track music files from memory into emulator.
 * @since 0.6.4
 */
BLARGG_EXPORT gme_err_t gme_load_tracks( Music_Emu* me,
                                         void const* data, long* sizes, int count );

/* Return the fixed track count of an emu file type
 * @since 0.6.4
 */
BLARGG_EXPORT int gme_fixed_track_count( gme_type_t );

/* Load music file using custom data reader function that will be called to
read file data. Most emulators load the entire file in one read call. */
typedef gme_err_t (*gme_reader_t)( void* your_data, void* out, int count );
BLARGG_EXPORT gme_err_t gme_load_custom( Music_Emu*, gme_reader_t, long file_size, void* your_data );

/* Load m3u playlist file from memory (must be done after loading music) */
BLARGG_EXPORT gme_err_t gme_load_m3u_data( Music_Emu*, void const* data, long size );


/******** User data ********/

/* Set/get pointer to data you want to associate with this emulator.
You can use this for whatever you want. */
BLARGG_EXPORT void  gme_set_user_data( Music_Emu*, void* new_user_data );
BLARGG_EXPORT void* gme_user_data( Music_Emu const* );

/* Register cleanup function to be called when deleting emulator, or NULL to
clear it. Passes user_data to cleanup function. */
typedef void (*gme_user_cleanup_t)( void* user_data );
BLARGG_EXPORT void gme_set_user_cleanup( Music_Emu*, gme_user_cleanup_t func );


#ifdef __cplusplus
	}
#endif

#endif
