/*
This is free and unencumbered software released into the public domain.

Anyone is free to copy, modify, publish, use, compile, sell, or
distribute this software, either in source code form or as a compiled
binary, for any purpose, commercial or non-commercial, and by any
means.

In jurisdictions that recognize copyright laws, the author or authors
of this software dedicate any and all copyright interest in the
software to the public domain. We make this dedication for the benefit
of the public at large and to the detriment of our heirs and
successors. We intend this dedication to be an overt act of
relinquishment in perpetuity of all present and future rights to this
software under copyright law.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

For more information, please refer to <https://unlicense.org>
*/

#include <assert.h>
#include <stdio.h>

#include <libusb.h>

const uint16_t HUAWEI_VENDOR_ID = 0x12d1;
const uint16_t HUAWEI_PRODUCT_ID = 0x1f01;

const unsigned char HUAWEI_MODESWITCH_MSG[] =
    { 0x55, 0x53, 0x42, 0x43, 0x12, 0x34, 0x56, 0x78, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x11, 0x06, 0x20, 0x00, 0x00, 0x01, 0x01, 0x00, 0x01,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

int main()
{
  int status = libusb_init(NULL);
  if (LIBUSB_SUCCESS != status) {
    fprintf(stderr,
            "libusb initialisation failed: %d (%s)\n",
            status,
            libusb_error_name(status));
    return 1;
  }

  libusb_device_handle *dev_handle =
      libusb_open_device_with_vid_pid(NULL,
                                      HUAWEI_VENDOR_ID,
                                      HUAWEI_PRODUCT_ID);
  if (NULL == dev_handle) {
    fprintf(stderr,
            "No USB device with vendor id 0x%04x / product id 0x%04x found!\n",
            HUAWEI_VENDOR_ID,
            HUAWEI_PRODUCT_ID);
    return 1;
  }

  libusb_device *dev = libusb_get_device(dev_handle);
  assert(NULL != dev);

  libusb_set_auto_detach_kernel_driver(dev_handle, 1);

  struct libusb_config_descriptor *config;
  status = libusb_get_active_config_descriptor(dev, &config);
  if (LIBUSB_SUCCESS != status) {
    fprintf(stderr,
            "Could not get active USB configuration descriptor: %d (%s)\n",
             status,
             libusb_error_name(status));
    return 1;
  }
  assert(NULL != config);

  if ((config->bNumInterfaces < 1) ||
      (config->interface[0].num_altsetting < 1)) {
    fputs("No USB interface / altsetting found!\n", stderr);
    return 1;
  }
  const struct libusb_interface_descriptor *ifdesc =
      config->interface[0].altsetting;
  status = libusb_claim_interface(dev_handle, ifdesc->bInterfaceNumber);
  if (LIBUSB_SUCCESS != status) {
    fprintf(stderr,
            "Could not claim USB interface: %d (%s)\n",
            status,
            libusb_error_name(status));
    return 1;
  }

  const uint8_t *ep = NULL;
  for (uint8_t i = 0; (NULL == ep) && (i < ifdesc->bNumEndpoints); ++i) {
    if (0 == (LIBUSB_ENDPOINT_IN & ifdesc->endpoint[i].bEndpointAddress)) {
      ep = &(ifdesc->endpoint[i].bEndpointAddress);
    }
  }
  if (NULL == ep) {
    fputs("No matching USB endpoint found!\n", stderr);
    return 1;
  }

  int transferred;
  status = libusb_bulk_transfer(dev_handle,
                                *ep,
                                (unsigned char *) HUAWEI_MODESWITCH_MSG,
                                sizeof(HUAWEI_MODESWITCH_MSG),
                                &transferred,
                                500);
  if (LIBUSB_SUCCESS != status) {
    fprintf(stderr,
            "Could not write modeswitch message: %d (%s)\n",
            status,
            libusb_error_name(status));
    return 1;
  }
  if (sizeof(HUAWEI_MODESWITCH_MSG) != transferred) {
    fprintf(stderr,
            "%d bytes of modeswitch message written (expected %u)\n",
            transferred,
            (unsigned) sizeof(HUAWEI_MODESWITCH_MSG));
    return 1;
  }

  return 0;
}
