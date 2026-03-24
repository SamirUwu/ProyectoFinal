#ifndef MY_REVERBSC_H
#define MY_REVERBSC_H

#define DSY_REVERBSC_MAX_SIZE 98936

typedef struct
{
    int    write_pos;
    int    buffer_size;
    int    read_pos;
    int    read_pos_frac;
    int    read_pos_frac_inc;
    int    dummy;
    int    seed_val;
    int    rand_line_cnt;
    float  filter_state;
    float *buf;
} ReverbScDl;

class MyReverb
{
  public:
    MyReverb() {}
    ~MyReverb() {}

    int Init(float sample_rate);
    int Process(const float &in1, const float &in2, float *out1, float *out2);

    inline void SetFeedback(const float &fb) { feedback_ = fb; }
    inline void SetLpFreq(const float &freq) { lpfreq_ = freq; }

  private:
    void NextRandomLineseg(ReverbScDl *lp, int n);
    int  InitDelayLine(ReverbScDl *lp, int n);

    float feedback_, lpfreq_;
    float sample_rate_;
    float damp_fact_;
    float prv_lpfreq_;
    int   init_done_;

    ReverbScDl delay_lines_[8];
    float aux_[DSY_REVERBSC_MAX_SIZE];
};

#endif