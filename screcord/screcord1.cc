/*
 * Copyright (C) 2013 Hermann Meyer
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 * --------------------------------------------------------------------------
 */

namespace screcord {

#define fmax(x, y) (((x) > (y)) ? (x) : (y))
#define fmin(x, y) (((x) < (y)) ? (x) : (y))

#define always_inline inline __attribute__((always_inline))

#define MAXRECSIZE 102400  //100kb
#define MAXFILESIZE INT_MAX-MAXRECSIZE // 2147352576  //2147483648-MAXRECSIZE

class SCapture {
private:
    SNDFILE *       recfile;
    int             fSamplingFreq;
    int             channel;
    float           *fcheckbox0;
    float           *fcheckbox1;
    float           *fformat;
    float           *fbargraph;
    float           *fbargraph1;
    int             IOTA;
    int             iA;
    int             savesize;
    int             filesize;
    float           *fRec0;
    float           *fRec1;
    float           *tape;
    sem_t           m_trig;
    pthread_t       m_pthr;
    int32_t         rt_prio;
    int32_t         rt_policy;
    volatile bool   keep_stream;
    bool            mem_allocated;
    bool            is_wav;
    bool            err;
    float           fConst0;
    float           fRecb0[2];
    int             iRecb1[2];
    float           fRecb2[2];
    float           fRecb0r[2];
    int             iRecb1r[2];
    float           fRecb2r[2];
    void        mem_alloc();
    void        mem_free();
    void        clear_state_f();
    int         activate(bool start);
    void        init(unsigned int samplingFreq);
    void        compute(int count, float *input0, float *output0);
    void        compute_st(int count, float *input0, float *input1, float *output0, float *output1);
    void        compute_quad(int count, float *input0, float *input1, float *input2, float *input3,
                                    float *output0, float *output1, float *output2, float *output3);
    void        save_to_wave(SNDFILE * sf, float *tape, int lSize);
    SNDFILE     *open_stream(std::string fname);
    void        close_stream(SNDFILE **sf);
    void        stop_thread();
    void        start_thread();
    void        disc_stream();
    inline std::string get_ffilename(); 
    void connect(uint32_t port,void* data);
    static void *run_thread(void* p);

public:
    LV2_State_Make_Path* make_path;
    void        set_thread_prio(int32_t prio, int32_t policy);
    static void clear_state(SCapture*);
    static int  activate_plugin(bool start, SCapture*);
    static void set_samplerate(unsigned int samplingFreq, SCapture*);
    static void mono_audio(int count, float *input0, float *output0, SCapture*);
    static void stereo_audio(int count, float *input0, float *input1, float *output0, float *output1, SCapture*);
    static void quad_audio(int count, float *input0, float *input1, float *input2, float *input3,
                            float *output0, float *output1, float *output2, float *output3, SCapture*);
    static void delete_instance(SCapture *p);
    static void connect_ports(uint32_t port,void* data, SCapture *p);
    SCapture(int channel_);
    ~SCapture();
};

template <class T>
inline std::string to_string(const T& t) {
    std::stringstream ss;
    ss << std::setfill('0') << std::setw(3) << t;
    return ss.str();
}

SCapture::SCapture(int channel_)
    : recfile(NULL),
      channel(channel_),
      fRec0(0),
      fRec1(0),
      tape(fRec0),
      m_pthr(0),
      rt_prio(0),
      rt_policy(0),
      keep_stream(false),
      mem_allocated(false),
      err(false) {
    sem_init(&m_trig, 0, 0);
    start_thread();
}

SCapture::~SCapture() {
    stop_thread();
    activate(false);
}

inline std::string SCapture::get_ffilename() {
    struct stat buffer;
    struct stat sb;
    std::string pPath;

#ifndef  __MOD_DEVICES__
    const char *Path = NULL;
    if (make_path) {
        Path = make_path->path(make_path->handle, "lv2record");
    }
    if ((Path != NULL) &&(strcmp(Path,"lv2record") != 0)) {
        pPath = Path;
        pPath +="/";
    } else {
        pPath = getenv("HOME");
        pPath +="/lv2record/";
    }
#else
    const char *Path = getenv("MOD_USER_FILES_DIR");
    pPath = Path ? Path : "/data/user-files";
    pPath += "/Audio Recordings/lv2record/";
#endif
    is_wav = int(*fformat) ? false : true;
    if (!(stat(pPath.c_str(), &sb) == 0 && S_ISDIR(sb.st_mode))) {
        mkdir(pPath.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    }

#ifndef  __MOD_DEVICES__
    std::string name = is_wav ?  "lv2_session199.wav" : "lv2_session199.ogg" ;
#else
    std::string name = is_wav ?  "mod_session199.wav" : "mod_session199.ogg" ;
#endif
    int i = 199;
    while (stat ((pPath+name).c_str(), &buffer) != 0) {
        name.replace(name.begin()+11,name.end()-4,to_string(i)); 
        i-=1;
        if (i < -1) break;
    }
    i+=2;
    name.replace(name.begin()+11,name.end()-4,to_string(i)); 
    return pPath+name;
}

void SCapture::disc_stream() {
    for (;;) {
        sem_wait(&m_trig);
        if (!recfile) {
            std::string fname = get_ffilename();
            recfile = open_stream(fname);
        }
        save_to_wave(recfile, tape, savesize);
        filesize +=savesize;
        if ((!keep_stream && recfile) || (filesize >MAXFILESIZE && is_wav)) {
            close_stream(&recfile);
            filesize = 0;
        }
    }
}

void SCapture::set_thread_prio(int32_t prio, int32_t policy) {
    rt_prio = prio;
    rt_policy = policy;
}

void *SCapture::run_thread(void *p) {
    (reinterpret_cast<SCapture *>(p))->disc_stream();
    return NULL;
}

void SCapture::stop_thread() {
    pthread_cancel (m_pthr);
    pthread_join (m_pthr, NULL);
}

void SCapture::start_thread() {
    pthread_attr_t      attr;
    struct sched_param  spar;
    if (rt_prio == 0) {
        rt_prio = sched_get_priority_max(SCHED_FIFO);
    }
    if ((rt_prio/5) > 0) rt_prio = rt_prio/5;
    spar.sched_priority = rt_prio;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_JOINABLE );
    pthread_setcancelstate (PTHREAD_CANCEL_ENABLE, NULL);
    if (rt_policy == 0) {
        pthread_attr_setschedpolicy(&attr, SCHED_FIFO);
    } else {
        pthread_attr_setschedpolicy(&attr, rt_policy);
    }
    pthread_attr_setschedparam(&attr, &spar);
    pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);
    pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
    // pthread_attr_setstacksize(&attr, 0x10000);
    if (pthread_create(&m_pthr, &attr, run_thread,
                       reinterpret_cast<void*>(this))) {
        err = true;
    }
    pthread_attr_destroy(&attr);
}

inline void SCapture::clear_state_f()
{
    for (int i=0; i<MAXRECSIZE; i++) fRec0[i] = 0;
    for (int i=0; i<MAXRECSIZE; i++) fRec1[i] = 0;
    for (int i=0; i<2; i++) fRecb0[i] = 0;
    for (int i=0; i<2; i++) iRecb1[i] = 0;
    for (int i=0; i<2; i++) fRecb2[i] = 0.0000003; // -130db
    for (int i=0; i<2; i++) fRecb0r[i] = 0;
    for (int i=0; i<2; i++) iRecb1r[i] = 0;
    for (int i=0; i<2; i++) fRecb2r[i] = 0.0000003; // -130db
}

void SCapture::clear_state(SCapture *p)
{
    static_cast<SCapture*>(p)->clear_state_f();
}

inline void SCapture::init(unsigned int samplingFreq)
{
    fSamplingFreq = samplingFreq;
    IOTA = 0;
    fConst0 = (1.0f / float(fmin(192000, fmax(1, fSamplingFreq))));
}

void SCapture::set_samplerate(unsigned int samplingFreq, SCapture *p)
{
    static_cast<SCapture*>(p)->init(samplingFreq);
}

inline void SCapture::save_to_wave(SNDFILE * sf, float *tape, int lSize)
{
    if (sf) {
        sf_write_float(sf,tape, lSize);
        sf_write_sync(sf);
    } else {
        err = true;
    }
}

SNDFILE *SCapture::open_stream(std::string fname)
{
    SF_INFO sfinfo ;
    sfinfo.channels = channel;
    sfinfo.samplerate = fSamplingFreq;
    sfinfo.format = is_wav ? SF_FORMAT_WAV | SF_FORMAT_FLOAT : SF_FORMAT_OGG | SF_FORMAT_VORBIS;
    
    SNDFILE * sf = sf_open(fname.c_str(), SFM_WRITE, &sfinfo);
    if (sf) return sf;
    else return NULL;
}

inline void SCapture::close_stream(SNDFILE **sf)
{
    if (*sf) sf_close(*sf);
    *sf = NULL;
}

void SCapture::mem_alloc()
{
    if (!fRec0) fRec0 = new float[MAXRECSIZE];
    if (!fRec1) fRec1 = new float[MAXRECSIZE];
    mem_allocated = true;
}

void SCapture::mem_free()
{
    mem_allocated = false;
    if (fRec0) { delete fRec0; fRec0 = 0; }
    if (fRec1) { delete fRec1; fRec1 = 0; }
}

int SCapture::activate(bool start)
{
    if (start) {
        if (!mem_allocated) {
            mem_alloc();
            clear_state_f();
        }
    } else if (mem_allocated) {
        mem_free();
    }
    return 0;
}

int SCapture::activate_plugin(bool start, SCapture *p)
{
    (p)->activate(start);
    return 0;
}

void always_inline SCapture::compute(int count, float *input0, float *output0)
{
    if (err) *fcheckbox0 = 0.0;
    int     iSlow0 = int(*fcheckbox0);
    *fcheckbox1 = int(fRecb2[0]);
    for (int i=0; i<count; i++) {
        float fTemp0 = (float)input0[i];
        
        float 	fRec3 = fmax(fConst0, fabsf(fTemp0));
        int iTemp1 = int((iRecb1[1] < 4096));
        fRecb0[0] = ((iTemp1)?fmax(fRecb0[1], fRec3):fRec3);
        iRecb1[0] = ((iTemp1)?(1 + iRecb1[1]):1);
        fRecb2[0] = ((iTemp1)?fRecb2[1]:fRecb0[1]);
        
        if (iSlow0) { //record
            if (iA) {
                fRec1[IOTA++] = fTemp0;
            } else {
                fRec0[IOTA++] = fTemp0;
            }
            if (IOTA > MAXRECSIZE-1) { // when buffer is full, flush to stream
                iA = iA ? 0 : 1 ;
                tape = iA ? fRec0 : fRec1;
                keep_stream = true;
                savesize = IOTA;
                sem_post(&m_trig);
                IOTA = 0;
            }
        } else if (IOTA) { // when record stoped, flush the rest to stream
            tape = iA ? fRec1 : fRec0;
            savesize = IOTA;
            keep_stream = false;
            sem_post(&m_trig);
            IOTA = 0;
            iA = 0;
        }
        output0[i] = fTemp0;
        // post processing
        fRecb2[1] = fRecb2[0];
        iRecb1[1] = iRecb1[0];
        fRecb0[1] = fRecb0[0];
    }
    *fbargraph = 20.*log10(fmax(0.0000003,fRecb2[0]));
}

void SCapture::mono_audio(int count, float *input0, float *output0, SCapture *p)
{
    (p)->compute(count, input0, output0);
}

void always_inline SCapture::compute_st(int count, float *input0, float *input1, float *output0, float *output1)
{
    if (err) *fcheckbox0 = 0.0;
    int iSlow0 = int(*fcheckbox0);
    *fcheckbox1 = int(fmax(fRecb2[0],fRecb2r[0]));
    for (int i=0; i<count; i++) {
        float fTemp0 = (float)input0[i];
        float fTemp1 = (float)input1[i];
        // check if we run into clipping
        float 	fRec3 = fmax(fConst0,fabsf(fTemp0));
        int iTemp1 = int((iRecb1[1] < 4096));
        fRecb0[0] = ((iTemp1)?fmax(fRecb0[1], fRec3):fRec3);
        iRecb1[0] = ((iTemp1)?(1 + iRecb1[1]):1);
        fRecb2[0] = ((iTemp1)?fRecb2[1]:fRecb0[1]);

        float 	fRec3r = fmax(fConst0,fabsf(fTemp1));
        int iTemp1r = int((iRecb1r[1] < 4096));
        fRecb0r[0] = ((iTemp1r)?fmax(fRecb0r[1], fRec3r):fRec3r);
        iRecb1r[0] = ((iTemp1r)?(1 + iRecb1r[1]):1);
        fRecb2r[0] = ((iTemp1r)?fRecb2r[1]:fRecb0r[1]);
        
        if (iSlow0) { //record
            if (iA) {
                fRec1[IOTA++] = fTemp0;
                fRec1[IOTA++] = fTemp1;
            } else {
                fRec0[IOTA++] = fTemp0;
                fRec0[IOTA++] = fTemp1;
            }
            if (IOTA > MAXRECSIZE-2) { // when buffer is full, flush to stream
                iA = iA ? 0 : 1 ;
                tape = iA ? fRec0 : fRec1;
                keep_stream = true;
                savesize = IOTA;
                sem_post(&m_trig);
                IOTA = 0;
            }
        } else if (IOTA) { // when record stoped, flush the rest to stream
            tape = iA ? fRec1 : fRec0;
            savesize = IOTA;
            keep_stream = false;
            sem_post(&m_trig);
            IOTA = 0;
            iA = 0;
        }
        output0[i] = fTemp0;
        output1[i] = fTemp1;
        // post processing
        fRecb2[1] = fRecb2[0];
        iRecb1[1] = iRecb1[0];
        fRecb0[1] = fRecb0[0];
        fRecb2r[1] = fRecb2r[0];
        iRecb1r[1] = iRecb1r[0];
        fRecb0r[1] = fRecb0r[0];
    }
    *fbargraph = 20.*log10(fmax(0.0000003,fRecb2[0]));
    *fbargraph1 = 20.*log10(fmax(0.0000003,fRecb2r[0]));
}

void SCapture::stereo_audio(int count, float *input0, float *input1, float *output0, float *output1, SCapture *p)
{
    (p)->compute_st(count, input0, input1, output0, output1);
}

void always_inline SCapture::compute_quad(int count, float *input0, float *input1, float *input2, float *input3,
                                        float *output0, float *output1, float *output2, float *output3)
{
    if (err) *fcheckbox0 = 0.0;
    int iSlow0 = int(*fcheckbox0);
    *fcheckbox1 = int(fmax(fRecb2[0],fRecb2r[0]));
    for (int i=0; i<count; i++) {
        float fTemp0 = (float)input0[i];
        float fTemp1 = (float)input1[i];
        float fTemp2 = (float)input2[i];
        float fTemp3 = (float)input3[i];
        // check if we run into clipping
        float 	fRec3 = fmax(fConst0,fabsf(fmax(fTemp0, fTemp2)));
        int iTemp1 = int((iRecb1[1] < 4096));
        fRecb0[0] = ((iTemp1)?fmax(fRecb0[1], fRec3):fRec3);
        iRecb1[0] = ((iTemp1)?(1 + iRecb1[1]):1);
        fRecb2[0] = ((iTemp1)?fRecb2[1]:fRecb0[1]);

        float 	fRec3r = fmax(fConst0,fabsf(fmax(fTemp1, fTemp3)));
        int iTemp1r = int((iRecb1r[1] < 4096));
        fRecb0r[0] = ((iTemp1r)?fmax(fRecb0r[1], fRec3r):fRec3r);
        iRecb1r[0] = ((iTemp1r)?(1 + iRecb1r[1]):1);
        fRecb2r[0] = ((iTemp1r)?fRecb2r[1]:fRecb0r[1]);
        
        if (iSlow0) { //record
            if (iA) {
                fRec1[IOTA++] = fTemp0;
                fRec1[IOTA++] = fTemp1;
                fRec1[IOTA++] = fTemp2;
                fRec1[IOTA++] = fTemp3;
            } else {
                fRec0[IOTA++] = fTemp0;
                fRec0[IOTA++] = fTemp1;
                fRec0[IOTA++] = fTemp2;
                fRec0[IOTA++] = fTemp3;
            }
            if (IOTA > MAXRECSIZE-4) { // when buffer is full, flush to stream
                iA = iA ? 0 : 1 ;
                tape = iA ? fRec0 : fRec1;
                keep_stream = true;
                savesize = IOTA;
                sem_post(&m_trig);
                IOTA = 0;
            }
        } else if (IOTA) { // when record stoped, flush the rest to stream
            tape = iA ? fRec1 : fRec0;
            savesize = IOTA;
            keep_stream = false;
            sem_post(&m_trig);
            IOTA = 0;
            iA = 0;
        }
        output0[i] = fTemp0;
        output1[i] = fTemp1;
        output2[i] = fTemp2;
        output3[i] = fTemp3;
        // post processing
        fRecb2[1] = fRecb2[0];
        iRecb1[1] = iRecb1[0];
        fRecb0[1] = fRecb0[0];
        fRecb2r[1] = fRecb2r[0];
        iRecb1r[1] = iRecb1r[0];
        fRecb0r[1] = fRecb0r[0];
    }
    *fbargraph = 20.*log10(fmax(0.0000003,fRecb2[0]));
    *fbargraph1 = 20.*log10(fmax(0.0000003,fRecb2r[0]));
}

void SCapture::quad_audio(int count, float *input0, float *input1, float *input2, float *input3,
                    float *output0, float *output1, float *output2, float *output3, SCapture *p)
{
    (p)->compute_quad(count, input0, input1, input2, input3, output0, output1, output2, output3);
}

void SCapture::connect(uint32_t port,void* data)
{
	switch ((PortIndex)port)
	{
	case FORM: 
		fformat = (float*)data; //  {{"wav"},{"ogg"},{0}};
		break;
	case REC: 
		fcheckbox0 = (float*)data; // , 0.0f, 0.0f, 1.0f, 1.0f 
		break;
	case CLIP: 
		fcheckbox1 = (float*)data; // , 0.0f, 0.0f, 1.0f, 1.0f 
		break;
	case LMETER: 
		fbargraph = (float*)data; // , -70.0, -70.0, 4.0, 0.00001 
		break;
	case RMETER: 
		fbargraph1 = (float*)data; // , -70.0, -70.0, 4.0, 0.00001 
		break;
	default:
		break;
	}
}

void SCapture::connect_ports(uint32_t port,void* data, SCapture *p)
{
	(p)->connect(port, data);
}

void SCapture::delete_instance(SCapture *p)
{
	delete (p);
}

} // end namespace gxrecord
