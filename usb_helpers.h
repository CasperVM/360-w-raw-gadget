#include "usb_descriptors.h"
#include "usb_io.h"

// Helper functions

// Build configuration descriptor
int build_config(char *data, int length, bool other_speed);
// Process non-control (not ep0) endpoints info call
void process_eps_info(int fd);
// Set a string descriptor on the io struct before send.
void set_usb_string_desc(const char *str, struct usb_raw_control_ep0_io *io);
// Log a control request
void log_control_request(struct usb_ctrlrequest *ctrl);
// Log a raw event
void log_event(struct usb_raw_event *event);
