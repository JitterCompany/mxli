//HEADER
#include <usb.h>

//SLICE
bool fifoPrintUsbDeviceClass(Fifo *fifo, int deviceClass) {
	switch(deviceClass) {
	case USB_CLASS_0:			return fifoPrintString(fifo,"USB_CLASS_0");			break;
	case USB_CLASS_AUDIO:			return fifoPrintString(fifo,"USB_CLASS_AUDIO");			break;
	case USB_CLASS_CDC:			return fifoPrintString(fifo,"USB_CLASS_CDC");			break;
	case USB_CLASS_HID:			return fifoPrintString(fifo,"USB_CLASS_HID");			break;
	case USB_CLASS_PHYSICAL:		return fifoPrintString(fifo,"USB_CLASS_PHYSICAL");		break;
	case USB_CLASS_IMAGE:			return fifoPrintString(fifo,"USB_CLASS_IMAGE");			break;
	case USB_CLASS_PRINTER:			return fifoPrintString(fifo,"USB_CLASS_PRINTER");		break;
	case USB_CLASS_MSD:			return fifoPrintString(fifo,"USB_CLASS_MSD");			break;
	case USB_CLASS_HUB:			return fifoPrintString(fifo,"USB_CLASS_HUB");			break;
	case USB_CLASS_CDC_DATA:		return fifoPrintString(fifo,"USB_CLASS_CDC_DATA");		break;
	case USB_CLASS_SMART_CARD:		return fifoPrintString(fifo,"USB_CLASS_SMART_CARD");		break;
	case USB_CLASS_CONTENT_SECURITY:	return fifoPrintString(fifo,"USB_CLASS_CONTENT_SECURITY");	break;
	case USB_CLASS_VIDEO:			return fifoPrintString(fifo,"USB_CLASS_VIDEO");			break;
	case USB_CLASS_PERSONAL_HEALTHCARE:	return fifoPrintString(fifo,"USB_CLASS_PERSONAL_HEALTHCARE");	break;
	case USB_CLASS_DIAGNOSTIC_DEVICE:	return fifoPrintString(fifo,"USB_CLASS_DIAGNOSTIC_DEVICE");	break;
	case USB_CLASS_WIRELESS_CONTROLLER:	return fifoPrintString(fifo,"USB_CLASS_WIRELESS_CONTROLLER");	break;
	case USB_CLASS_MISCELLANEOUS:		return fifoPrintString(fifo,"USB_CLASS_MISCELLANEOUS");		break;
	case USB_CLASS_APPLICATION_SPECIFIC:	return fifoPrintString(fifo,"USB_CLASS_APPLICATION_SPECIFIC");	break;
	case USB_CLASS_VENDOR_SPECIFIC:		return fifoPrintString(fifo,"USB_CLASS_VENDOR_SPECIFIC");	break;
	default: return fifoPrintString(fifo,"Unknown device class");
	}
}


//SLICE
bool fifoPrintUsbDescriptorType(Fifo *fifo, int descriptorType) {
	switch(descriptorType) {
	case USB_DESCRIPTOR_DEVICE:		return fifoPrintString(fifo,"USB_DESCRIPTOR_DEVICE");		break;
	case USB_DESCRIPTOR_CONFIGURATION:	return fifoPrintString(fifo,"USB_DESCRIPTOR_CONFIGURATION");	break;
	case USB_DESCRIPTOR_STRING:		return fifoPrintString(fifo,"USB_DESCRIPTOR_STRING");		break;
	case USB_DESCRIPTOR_INTERFACE:		return fifoPrintString(fifo,"USB_DESCRIPTOR_INTERFACE");	break;
	case USB_DESCRIPTOR_ENDPOINT:		return fifoPrintString(fifo,"USB_DESCRIPTOR_ENDPOINT");		break;
	case USB_DESCRIPTOR_DEVICE_QUALIFIER:	return fifoPrintString(fifo,"USB_DESCRIPTOR_DEVICE_QUALIFIER");	break;
	case USB_DESCRIPTOR_OTHER_SPEED_CONFIGURATION:
				return fifoPrintString(fifo,"USB_DESCRIPTOR_OTHER_SPEED_CONFIGURATION");	break;
	case USB_DESCRIPTOR_INTERFACE_POWER:	return fifoPrintString(fifo,"USB_DESCRIPTOR_INTERFACE_POWER");	break;
	default: return fifoPrintString(fifo,"Unknown descriptor type");
	}
}

//SLICE
bool fifoPrintUsbDeviceRequest(Fifo *fifo, UsbDeviceRequest const *packet) {
	fifoPrintString(fifo,"UsbDeviceRequest{");
	fifoPrintString(fifo,(packet->type & 1<<7) ? "device->host" : "host->device");
	fifoPrintString(fifo,",request=");
	switch(packet->request) {
		case USB_GET_STATUS:		fifoPrintString(fifo,"USB_GET_STATUS");		break;
		case USB_CLEAR_FEATURE:		fifoPrintString(fifo,"USB_CLEAR_FEATURE");	break;
		case USB_RESERVED_1:		fifoPrintString(fifo,"USB_RESERVED_1");		break;
		case USB_SET_FEATURE:		fifoPrintString(fifo,"USB_SET_FEATURE");	break;
		case USB_RESERVED_2:		fifoPrintString(fifo,"USB_RESERVED_2");		break;
		case USB_SET_ADDRESS:		fifoPrintString(fifo,"USB_SET_ADDRESS");	break;
		case USB_GET_DESCRIPTOR:	fifoPrintString(fifo,"USB_GET_DESCRIPTOR");	break;
		case USB_SET_DESCRIPTOR:	fifoPrintString(fifo,"USB_SET_DESCRIPTOR");	break;
		case USB_GET_CONFIGURATION:	fifoPrintString(fifo,"USB_GET_CONFIGURATION");	break;
		case USB_SET_CONFIGURATION:	fifoPrintString(fifo,"USB_SET_CONFIGURATION");	break;
		case USB_GET_INTERFACE:		fifoPrintString(fifo,"USB_GET_INTERFACE");	break;
		case USB_SET_INTERFACE:		fifoPrintString(fifo,"USB_SET_INTERFACE");	break;
		case USB_SYNC_FRAME:		fifoPrintString(fifo,"USB_SYNC_FRAME");		break;
		default:			fifoPrintString(fifo,"undefined.");
	}
		
	fifoPrintString(fifo,",type=");
	switch(packet->type>>5 & 3) {
		case 0:	fifoPrintString(fifo,"standard"); break;
		case 1: fifoPrintString(fifo,"class"); break;
		case 2: fifoPrintString(fifo,"vendor"); break;
		default: fifoPrintString(fifo,"reserved");
	}
	fifoPrintString(fifo,",recipient=");
	switch(packet->type&0x1F) {
		case 0: fifoPrintString(fifo,"device"); break;
		case 1: fifoPrintString(fifo,"interface"); break;
		case 2: fifoPrintString(fifo,"endpoint"); break;
		case 3: fifoPrintString(fifo,"other"); break;
		default: fifoPrintString(fifo,"reserved");
	}
	return fifoPrintString(fifo,"}\n");
}

//SLICE
bool fifoPrintUsbSetup (Fifo *fifo, const UsbSetup *p) {
	const int recipient = p->requestType>>_USB_SETUP_REQUESTTYPE_RECIPIENT & 0x1F;
	const int type = p->requestType>>_USB_SETUP_REQUESTTYPE_TYPE & 0x3;
	const int dir = p->requestType>>_USB_SETUP_REQUESTTYPE_DIRECTION & 1;
	fifoPrintString (fifo, "UsbSetup { recipient=");
	switch (recipient) {
		case 0: fifoPrintString (fifo,"device"); break;
		case 1: fifoPrintString (fifo,"interface"); break;
		case 2: fifoPrintString (fifo,"endpoint"); break;
		case 3: fifoPrintString (fifo,"other"); break;
		default: fifoPrintString (fifo,"reserved"); break;
	}
	fifoPrintString (fifo,", request=");
	switch(type) {
		case USB_TYPE_STANDARD:	fifoPrintString (fifo,"standard"); break;
		case USB_TYPE_CLASS:	fifoPrintString (fifo,"class"); break;
		case USB_TYPE_VENDOR:	fifoPrintString (fifo,"vendor"); break;
		default: fifoPrintString (fifo,"reserved");
	}
	fifoPrintString (fifo,", direction=");
	fifoPrintString (fifo, dir ? "device to host" : "host to device");
	fifoPrintString (fifo,", request=0x"); fifoPrintHex (fifo, p->request, 2,2);
	fifoPrintChar (fifo,'=');
	switch(p->request) {
		case USB_GET_STATUS:		fifoPrintString (fifo,"get status"); break;
		case USB_CLEAR_FEATURE:		fifoPrintString (fifo,"clear feature"); break;
		case USB_SET_FEATURE:		fifoPrintString (fifo,"set feature"); break;
		case USB_SET_ADDRESS:		fifoPrintString (fifo,"set address"); break;
		case USB_GET_DESCRIPTOR:	fifoPrintString (fifo,"get descriptor"); break;
		case USB_SET_DESCRIPTOR:	fifoPrintString (fifo,"set descriptor"); break;
		case USB_GET_CONFIGURATION:	fifoPrintString (fifo,"get configuration"); break;
		case USB_SET_CONFIGURATION:	fifoPrintString (fifo,"set configuration"); break;
		case USB_GET_INTERFACE:		fifoPrintString (fifo,"get interface"); break;
		case USB_SET_INTERFACE:		fifoPrintString (fifo,"set interface"); break;
		case USB_SYNC_FRAME:		fifoPrintString (fifo,"sync frame"); break;
		default: 			fifoPrintString (fifo,"reserved/unknown");
	}
	fifoPrintString (fifo,", value=0x"); fifoPrintHex (fifo, p->value, 4,4);
	fifoPrintString (fifo,", index=0x"); fifoPrintHex (fifo, p->index, 4,4);
	fifoPrintString (fifo,", count=0x"); fifoPrintHex (fifo, p->count, 4,4);
	return fifoPrintString (fifo, " }");
}

