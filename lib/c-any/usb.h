#ifndef usb_h
#define usb_h

/** @file
 * @brief Common USB constants and structures.
 */

#include <stdbool.h>
#include <integers.h>
#include <fifoPrint.h>
#include <unionized.h>

/** Most device classes can only be used on the interface level, not the device level.
 */
enum UsbDeviceClass {
	USB_CLASS_0 =0x00,		///< indicates that the interfaces define the class rather than the device.
	USB_CLASS_AUDIO =0x01,			///< (interface)
	USB_CLASS_CDC =0x02,			///< (device and interface) Communication device class.
	USB_CLASS_HID =0x03,			///< (interface) human interface device
	USB_CLASS_PHYSICAL =0x05,		///< (interface)
	USB_CLASS_IMAGE =0x06,			///< (interface)
	USB_CLASS_PRINTER =0x07,		///< (interface)
	USB_CLASS_MSD =0x08,			///< (interface) mass storage devices
	USB_CLASS_HUB =0x09,			///< (device) USB hub
	USB_CLASS_CDC_DATA =0x0A,		///< (interface)
	USB_CLASS_SMART_CARD =0x0B,		///< (interface)
	USB_CLASS_CONTENT_SECURITY=0x0D,	///< (interface)
	USB_CLASS_VIDEO =0x0E,			///< (interface)
	USB_CLASS_PERSONAL_HEALTHCARE =0x0F,	///< (interface)
	USB_CLASS_DIAGNOSTIC_DEVICE =0xDC,	///< (device and interface)
	USB_CLASS_WIRELESS_CONTROLLER =0xE0,	///< (interface)
	USB_CLASS_MISCELLANEOUS =0xEF,		///< (device and interface)
	USB_CLASS_APPLICATION_SPECIFIC =0xFE,	///< (interface)
	USB_CLASS_VENDOR_SPECIFIC =0xFF,	///< (device and interface)
};

bool fifoPrintUsbDeviceClass(Fifo *fifo, int deviceClass);

typedef struct __attribute__((packed)) {
	Uint8	type;		///< direction [7] (1=in), type [6:5] and recipient [3:0]
	Uint8	request;	///< Usb request code
	Uint16	value;
	Uint16	index;
	Uint16	length;
} UsbDeviceRequest;

enum UsbRequestType {
	USB_TYPE_STANDARD,
	USB_TYPE_CLASS,
	USB_TYPE_VENDOR,
};

enum UsbRequestCode {
	USB_GET_STATUS		=0,
	USB_CLEAR_FEATURE	=1,
	USB_RESERVED_1,
	USB_SET_FEATURE		=3,
	USB_RESERVED_2,
	USB_SET_ADDRESS		=5,
	USB_GET_DESCRIPTOR	=6,
	USB_SET_DESCRIPTOR	=7,
	USB_GET_CONFIGURATION	=8,
	USB_SET_CONFIGURATION	=9,
	USB_GET_INTERFACE	=10,
	USB_SET_INTERFACE	=11,
	USB_SYNC_FRAME		=12,
};

enum UsbDescriptorType {
	USB_DESCRIPTOR_DEVICE		=1,
	USB_DESCRIPTOR_CONFIGURATION	=2,
	USB_DESCRIPTOR_STRING		=3,
	USB_DESCRIPTOR_INTERFACE	=4,
	USB_DESCRIPTOR_ENDPOINT		=5,
	USB_DESCRIPTOR_DEVICE_QUALIFIER	=6,
	USB_DESCRIPTOR_OTHER_SPEED_CONFIGURATION	=7,
	USB_DESCRIPTOR_INTERFACE_POWER	=8,
};

bool fifoPrintUsbDescriptorType(Fifo *fifo, int descriptorType);

/** The device descriptor provides general information about the device.
 * Exactly one device descriptor exists for a device.
 */
typedef struct PACKED4 {
	Uint8	length;		///< size of this data structure
	Uint8	type;		///< descriptor type
	Uint16	usb;		///< USB version as BCD. 2.0 -> 0x200
	Uint8	deviceClass;
	Uint8	deviceSubclass;
	Uint8	deviceProtocol;
	Uint8	maxPacketSize0;	///< endpoint 0 packet size: 8,16,32 or 64.
	Uint16	idVendor;
	Uint16	idProduct;
	Uint16	release;	///< device release number as BCD
	Uint16	indexManufacturer;
	Uint16	indexProduct;
	Uint16	indexSerialNumber;
	Uint8	configurations;	///< number of possible configurations
} UsbDeviceDescriptor;

bool fifoPrintUsbDeviceRequest(Fifo *fifo, UsbDeviceRequest const *packet);

typedef struct PACKED4 {
	Uint8		requestType;
	Uint8		request;
	Uint16		value;
	Uint16		index;
	Uint16		count;
} UsbSetup;

enum UsbSetupBits {
	_USB_SETUP_REQUESTTYPE_RECIPIENT	=0,	///< 5 bits, 0=device, 1=interface, 2=EP, 3=other, other reserved.
	_USB_SETUP_REQUESTTYPE_TYPE		=5,	///< 2 bits, 0=standard, 1=class, 2=vendor, 3=reserved
	_USB_SETUP_REQUESTTYPE_DIRECTION	=7,	///< 1 bit, 0=host to device, 1=device to host
};

/** Prints detailed information about the setup packed.
 */
bool fifoPrintUsbSetup (Fifo *fifo, const UsbSetup *p);

#endif
