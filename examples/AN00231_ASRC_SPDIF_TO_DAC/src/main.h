#ifndef MAIN_H_
#define MAIN_H_

//Interface for obtaining timing information about the serial stream
typedef interface sample_rate_enquiry_if {
    unsigned get_sample_count(int &elapsed_time_in_ticks); //Returns sample count and elapsed time in 10ns ticks since last query
    {unsigned, unsigned } get_buffer_level(void);          //Returns the buffer level. Note this will return zero for the serial2block function which uses a double-buffer
} sample_rate_enquiry_if;

//Interface for serveing up the last calculated fs_ratio between input and output
typedef interface fs_ratio_enquiry_if {
    unsigned get_ratio(unsigned nominal_fs_ratio);      //Get the current ratio
    [[notification]] slave void new_sr_notify(void);
    [[clears_notification]] unsigned get_in_fs(void);   //Get the nominal sample rate
    [[clears_notification]] unsigned get_out_fs(void);
} fs_ratio_enquiry_if;

//Sets or clears pixel. Origin is bottom left scanning right
typedef interface led_matrix_if {
    void set(unsigned col, unsigned row, unsigned val);
} led_matrix_if;

//Notifies and sends a button value when a button is pressed
typedef interface buttons_if {
    void pressed(unsigned button_idx);
} buttons_if;

//Type which tells us the current status of the detected sample rate
//Is it with range of a supported rate?
typedef enum sample_rate_status_t{
    INVALID,
    VALID }
    sample_rate_status_t;

//Q4.28 fixed point frequency ratio type
typedef unsigned fs_ratio_t;

#endif /* MAIN_H_ */
