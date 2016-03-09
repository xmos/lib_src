#ifndef MAIN_H_
#define MAIN_H_

typedef interface sample_rate_enquiry_if {
    unsigned get_sample_count(int &elapsed_time_in_ticks);
    unsigned get_buffer_level(void);
} sample_rate_enquiry_if;

typedef interface fs_ratio_enquiry_if {
    unsigned get(unsigned nominal_fs_ratio);
    [[notification]] slave void new_sr_notify(void);
    [[clears_notification]] unsigned get_in_fs(void);
    [[clears_notification]] unsigned get_out_fs(void);
} fs_ratio_enquiry_if;

//Sets or clears pixel. Origin is bottom left scanning right
typedef interface led_matrix_if {
    void set(unsigned col, unsigned row, unsigned val);
} led_matrix_if;

//Type which tells us the current status of the detected sample rate
//Is it with range of a supported rate?
typedef enum sample_rate_status_t{
    INVALID,
    VALID }
    sample_rate_status_t;

//Q4.28 fixed point frequency ratio type
typedef unsigned fs_ratio_t;

#endif /* MAIN_H_ */
