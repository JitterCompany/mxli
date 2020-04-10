#ifndef __miim_h
#define __miim_h

#include <stdbool.h>
#include <integers.h>
#include <fifo.h>

/** @file
 * @brief MIIM Standard definitions.
 *
 * MIIM is a protocol used for configuration and status exchange between MII/RMII MACs and PHYs.
 * Clause 22 MIIM can access up to 32 registers in up to 32 devices.
 * In Bit definitions: RO, RW, SC=self clearing, LH=latch high (=1 until read), LL=latch low, COR=clear on read.
 * <p>
 * To be honest, MIIM is bullshit, since there is too much freedom of how bits are represented. Therefore I define a
 * few interface functions, that provide the BASIC functionality.
 * <p>
 * Use MIIM to configure the PHY chip, then configure the MAC to match this configuration. Configuration includes:
 * <ul>
 *   <li>auto-negotiation <li>advertizing of mode <li>auto MDI-X <li>LED indicators mode
 *   <li>fixed configuration 10/100Mb <li>full/half duplex
 * </ul>
 * Suppression of preamble is a fixed feature of the PHY - you can-t configure it.
 * MIIM further allows power down and reset of the PHY, but I'd rather prefer hardwired signals for this.
 */

/** Writes a PHY register - blocking.
 * This is an interface function to be implemented by the macInit() function module.
 * @param phyAddress the PHYs address in the range 0..31
 * @param registerAddress the destination register in the range 0..31
 * @param value the 16-bit value to write. Unused bits must be 0
 * @return true if write was successful.
 */
bool miimWrite(int phyAddress, int registerAddress, Uint16 value);

/** Reads a PHY register - blocking.
 * This is an interface function to be implemented by the macInit() function module.
 * @param phyAddress the PHYs address in the range 0..31
 * @param registerAddress the source register in the range 0..31
 * @param value the destination for the 16-bit value of the register. Unused bits must will read as 0
 * @return true if read was successful
 */
bool miimRead(int phyAddress, int registerAddress, Uint16* value);

/** Checks if Ethernet cable is plugged in and signal energy is detected.
 * This is an interface function to be implemented by the macInit() function module.
 * Different PHY chips have different ways of determinating this and latch low behaviour of BasicStatus is not very
 * helpful.
 */
bool miimHasLink(char phyAddress);

/** This structure is provided to allow storage of a readable form if PhyIdentifier1/2 where OUI-bits are permuted
 * like crazy.
 */
typedef struct {
	long	oui;
	char	vendorModel;
	char	modelRevision;
} MiimIdentifier;

/** Tests equality of two identifiers.
 */
inline static bool miimIdentifierEqual(const MiimIdentifier *i1, const MiimIdentifier *i2) {
	return	i1->oui==i2->oui
		&& i1->vendorModel==i2->vendorModel
		&& i1->modelRevision==i2->modelRevision;
}

enum {
	OUI_NATIONAL_SEMICONDUCTOR	=0x080017,
	//OUI_MICREL			=0x0010A1,	// used in Micrels PHY 8721
	OUI_MICREL			=0x000885,	// used in Micrels PHY 8721

	MIC8721_OUI			=OUI_MICREL,
	MIC8721_MODEL			=0x21,
	DP83484_OUI			=OUI_NATIONAL_SEMICONDUCTOR,		
	DP83484_MODEL			=0x24,
};

/** Reads the PHY's identifier.
 */
bool miimGetIdentifier(int addressPhy, MiimIdentifier* identifier);

/** Writes the PHY identifier information.
 */
bool fifoPrintMiimIdentifier(Fifo *fifo, const MiimIdentifier* identifier);

/** Searches for the address of a given PHY.
 * @param pattern a search pattern for the PHY identifier. OUI, vendor and model can be defined or set to -1 to match
 *   all. Do not set all three values to -1.
 * @return the address of the first PHY that matches the pattern (starting at low addresses) or -1 if not found.
 */
int miimScanAddress(const MiimIdentifier* pattern);

/** PHYs are so different that no there's no way to extract basic information in a uniform manner. Therefor we
 * define a structure of basic Information here and rely on a PHY-specific driver to provide this information. MACs
 * need this information for proper communication with the PHY.
 */
typedef struct {
	int	valid;			///< the following link properties are valid
//	int	up:1;			///< 1=link is up
	int	speed100:1;		///< 1 for 100Mbps, 0 for 10Mbps
	int	fullDuplex:1;		///< 1 for full duplex, 0 for half duplex
	int	pause:1;		///< 1 if PAUSE frames to be used
} MiimLinkProperties;

/** Prints link properties in a human-friendly way.
 * @param out output destination
 * @param linkProperties the link properties
 * @return true if successfully printed, false if buffer overflown.
 */
bool fifoPrintMiimLinkProperties(Fifo *out, const MiimLinkProperties linkProperties);

/** A driver module must provide this function.
 * @return the current status of the PHY.
 */
MiimLinkProperties miimGetLinkProperties(int addressPhy);

/** Checks, if auto negotiation was automatically started by the PHY and if not starts an auto-negotiation process now.
 * If the PHY does not support auto-negotiation, then this function returns immediately successful.
 * @param addressPhy the PHY's address in the range 0..31
 * @return true if auto-negotiation was started (or interface is fixed) or false if an error occurred.
 */
bool miimAutoNegotiationStart(int addressPhy);

/** Checks if auto negotiation is completed.
 * @return true if auto-negotiation is completed.
 */
bool miimAutoNegotiationCompleted(int addressPhy);

/** Checks if link is lost.
 * @return true if link went down since last call of this function. This is NOT a reliable means of checking for
 *   an established link.
 */
bool miimLinkLost(int addressPhy);

enum {
	MIIM_BASIC_CONTROL=0,
	MIIM_BASIC_STATUS=1,
	MIIM_IDENTIFIER1=2,
	MIIM_IDENTIFIER2=3,
	MIIM_AUTO_NEGOTIATION_ADVERTISEMENT=4,
	MIIM_AUTO_NEGOTIATION_LINK_PARTNER_ABILITY=5,
	MIIM_AUTO_NEGOTIATION_EXPANSION=6,
	MIIM_AUTO_NEGOTIATION_NEXT_PAGE=7,
	MIIM_AUTO_NEGOTIATION_NEXT_PAGE_ABILITY=8,
} MiimRegister;

////////////////////////////////////////////////////////////////////////////////
// low level stuff

/** Bits of Basic Control register. Devices: DP83848, KD8721
 */
typedef union {
	struct __attribute__((packed)) { Uint16
		disableTransmitter:1,		// 8721 only; =0 as default
		:6,
		collisionTest:1,
		duplexMode:1,			// 8721, 83844
		restartAutoNegotiation:1,
		isolate:1,			///< isolate PHY, MIIM remains
		powerDown:1,			///< if =1, the device is powered down, MIIM remains
		autoNegotiationEnable:1,	///< overrides duplexMode..speedSelection when set.
		speedSelection:1,		///< 1=100Mbps, 0=10Mbps
		loopBack:1,			///< route transmit data to the receive data path.
		reset:1;			///< SC, Reads as 1 until reset finish.
	};
	Uint16		uint16;
} MiimBasicControl; // 0x00 BMCR

/** Convenience function.
 */
static inline bool miimWriteBasicControl(int addressPhy, MiimBasicControl control) {
	return miimWrite(addressPhy,MIIM_BASIC_CONTROL,control.uint16);
}

/** Convenience function.
 */
static inline MiimBasicControl miimReadBasicControl(int addressPhy) {
	MiimBasicControl control = { } ;
	miimRead(addressPhy,MIIM_BASIC_CONTROL, &control.uint16);
	return control;
}

/** Outputs all BasicControl fields.
 * @param fifo the output destination
 * @param status the information to print in a human-readable format.
 * @param separator the separator string printed in front of each field except the first.
 * @return true if output is complete, false if output was truncated.
 */
bool fifoPrintMiimBasicControl(Fifo *fifo, const MiimBasicControl status, const char* separator);

typedef union {
	struct __attribute__((packed)) { Uint16
		extendedCapability:1,		///< RO; device supports extended capabilities registers.
		jabberDetect:1,			///< RO,LH; 10Mbit only: jabber detected.
		linkStatus:1,			///< RO,LL; valid link established, 0=link lost. Read manual about LL this.
		autoNegotiationAbility:1,	///< RO; device supports auto-negotiation
		remoteFault:1,			///< RO,LH; remote fault condition detected.
		autoNegotiationComplete:1,	///< RO; 1=finished
		preambleSuppression:1,		///< RO; device can suppress preamble. Read manual about this.
		:4,
		halfDuplex10BaseT:1,		///< RO; device supports 10BaseT half duplex.
		fullDuplex10BaseT:1,
		halfDuplex100BaseTx:1,
		fullDuplex100BaseTx:1,
		_100BaseT4:1;
	};
	Uint16	uint16;
} MiimBasicStatus; // 0x01 BMSR

/** Reads the basic status of the PHY.
 * @param addressPhy the probed address of the PHY device.
 * @param basicStatus a pointer to the result variable.
 * @return true if PHY successfully read, false otherwise.
 */
static inline bool miimReadBasicStatus(int addressPhy, MiimBasicStatus *basicStatus) {
	return miimRead(addressPhy,MIIM_BASIC_STATUS,&basicStatus->uint16);
}

/** Outputs all BasicStatus fields.
 * @param fifo the output destination
 * @param status the information to print in a human-readable format.
 * @param separator the separator string printed in front of each field except the first.
 * @return true if output is complete, false if output was truncated.
 */
bool fifoPrintMiimBasicStatus(Fifo *fifo, const MiimBasicStatus status, const char* separator);

typedef union {
	Uint16	ouiBits17downto2;	// bits reversed and byte order reversed. WTF...
	Uint16	uint16;
} MiimPhyIdentifier1;	// 0x02 PHYIDR1

typedef union {
	struct __attribute__((packed)) {
		Uint16
			modelRevision:4,
			vendorModel:6,
			ouiBits23downto18:6;	// bits reversed and byte order reversed. WTF...
	};
	Uint16	uint16;
} MiimPhyIdentifier2; // 0x03 PHYIDR2

/** Auto negotiation advertisement.
 */
typedef union {
	struct __attribute((packed)) { Uint16
		selector:5,			///< RW; protocol selection, 1=IEEE 802.3u
		_10BaseT:1,
		fullDuplex10BaseT:1,
		_100BaseTx:1,
		fullDuplex100BaseTx:1,
		_100BaseT4:1,			///< RO=0
		pause:1,			///< RW; PAUSE support for full duplex links (flow control)
		asymmetricPause:1,		///< 83848 only: assymetric pause support. Read manual.
		:1,
		remoteFault:1,
		:1,
		nextPage:1;
	};
	Uint16	uint16;
} MiimAna;	 // 0x04 ANAR

bool fifoPrintMiimAna(Fifo *fifo, const MiimAna ana, const char* separator);

/** Auto negotiation link partner ability.
 */
typedef union {
	struct __attribute__((packed)) { Uint16
		selector:5,
		_10BaseT:1,
		fullDuplex10BaseT:1,
		_100BaseTx:1,
		fullDuplex100BaseTx:1,
		_100BaseT4:1,			///< RO=0
		pause:1,			///< PAUSE support for full duplex links (flow control)
		asymmetricPause:1,		///< 83848 only: asymetric pause support. Read manual.
		:1,
		remoteFault:1,
		acknowledge:1,			///< link partner ack to ability data word.
		nextPage:1;			///< link partner desires next page transfer.
	};
	Uint16	uint16;
} MiimAnlpa; // 0x05 ANLPAR

bool fifoPrintMiimAnlpa(Fifo *fifo, const MiimAnlpa anlpa, const char* separator);

/** The OUI (Organizationally Unique Identifier) is a 24-bit value.
 */
typedef union {
	struct __attribute__((packed)) { Uint32
		bits0to1:2,
		bits2to17:16,
		bits18to23:7;
	};
	Uint32	uint32;	
} MiimOui;

/** Auto negotiation Expansion.
 */
typedef union {
	struct __attribute__((packed)) { Uint16
		linkPartnerAutoNegotiationAble:1,	///< link partner supports auto-negotiation
		linkCodeWordReceived:1,			///< RO,COR; link code word received
		nextPageAble:1,				///< RO; local device is able to send additional next pages
		linkPartnerNextPageAble:1,		///< RO; link partner does support next page
		parallelDetectionFault:1,
		:11;
	};
	Uint16	uint16;
} MiimAne; // 0x06 ANER

/** Auto negotiation next page.
 */
typedef union {
	struct __attribute__((packed)) { Uint16
		code:11,				///< RW, def=1; code field, 1=Null Page, messagePage=1
		toggle:1,
		acknowledge2:1,				///< will comply with message
		messagePage:1,				///< RW; 1=message page, 0=unformatted.
		acknowledge:1,				///< 8721 only: successful receive of link word
		nextPage:1;				///< other next page transfer desired.
	};
	Uint16	uint16;
} MiimAnnp; // 0x07 ANNPTR

#endif
