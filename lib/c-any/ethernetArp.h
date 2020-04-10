#ifndef __ethernetArp_h
#define __ethernetArp_h

/** @file
 * @brief ARP over ethernet packets data structures, cache and protocol helpers.
 *
 */

#include <stdbool.h>
#include <integers.h>
#include <fifo.h>
#include <ethernet.h>
#include <ip4.h>
#include <ioFifo.h>

/** Ethernet ARP Header. This structure contains all fields in little endian order -  read/write operations
 * must ensure change in endianess.
 */
typedef struct __attribute__((packed,aligned(2))) {
		Uint16		hardwareAddressType;	///< 1=Ethernet
		Uint16		protocol;		///< for which protocol ? 0x0800 = IP
		Uint8		hardwareAddressSize;	///< 6 for Ethernet
		Uint8		protocolAddressSize;	///< 4 for IPv4
		Uint16		operation;		///< 1=request, 2=response
		EthernetAddress	sourceEthernetAddress;
		Ip4Address	sourceIp4Address;
		EthernetAddress	destinationEthernetAddress;
		Ip4Address	destinationIp4Address;
		Uint8		data[];
} EthernetArpHeader;

enum {
	ETHERNET_ARP_OP_REQUEST		=1,
	ETHERNET_ARP_OP_REPLY		=2,
	ETHERNET_ARP_OP_RARP_REQUEST	=3,
	ETHERNET_ARP_OP_RARP_REPLY	=4,
	ETHERNET_ARP_OP_DRARP_REQUEST	=5,
	ETHERNET_ARP_OP_DRARP_REPLY	=6,
	ETHERNET_ARP_OP_DRARP_ERROR	=7,
	ETHERNET_ARP_OP_INARP_REQUEST	=8,
	ETHERNET_ARP_OP_INARP_REPLY	=9,
};

/** Prints a valid ARP request/response.
 * @param fifo output destination
 * @param header the header of the ARP packet.
 * @return true if fifo buffer was large enough to hold the printed chars.
 */
bool fifoPrintEthernetArpHeader(Fifo *fifo, const EthernetArpHeader* header);

/** Extracts an ARP-over-Ethernet header from a binary stream in network byte order.
 * @return true in case of success, false otherwise (and fifo unchanged).
 */
bool fifoGetnetEthernetArpHeader(Fifo *fifo, EthernetArpHeader *header);

/** Writes an ARP-over-Ethernet header to a binary stream in network byte order.
 * @return true in case of success, false otherwise (and fifo unchanged).
 */
bool fifoPutnetEthernetArpHeader(Fifo *fifo, const EthernetArpHeader *header);

typedef enum {
	ETHERNET_ARP_INVALID=0,			///< uninitialized table entry
	ETHERNET_ARP_UNRESOLVED=1,		///< the cache was asked for, but could not resolve
	ETHERNET_ARP_REQUESTED=2,		///< the UNRESOLVED entry was extracted, now waiting for setting of the value
	ETHERNET_ARP_RESOLVED=4,		///< ARP response received
	ETHERNET_ARP_RESOLVED_REQUESTED=6,	///< resolved but refresh in action
} EthernetArpEntryState;

typedef struct {
	Ip4Address		ip4Address;
	EthernetAddress		ethernetAddress;
	int			time;			///< time of installation of entry
	int			timeLookup;		///< time of latest lookup
	EthernetArpEntryState	state;			///< 0==ETHERNET_ARP_INVALID indicates unused entry
} EthernetArpCacheEntry;

/** Prints a mapping IP-address to EthernetAddress.
 * @param outputStream the printing destination
 * @param entry a valid or invalid cache entry.
 * @return true, if outputStream took all characters, false otherwise.
 */
bool fifoPrintEthernetArpCacheEntry(Fifo *outputStream, const EthernetArpCacheEntry *entry);

/** An IP-address to Ethernet-address lookup table. Mappings have a time stamp of the insertion time and expire after
 * a configurable time. Unsuccessful lookups are recorded as unresolved and can be extracted for generating ARP requests
 * automatically. Each IP-address generates 1 entry at most, which can change from the states UNRESOLVED to REQUESTED
 * to RESOLVED and back to INVALID after expiry. In case of cache overflow, the least recently used entries are re-used,
 * independent on the specific state. If an entry is periodically looked up, but never updated it will be returned as
 * for refresh after the predefined refresh time.
 */
typedef struct EthernetArpCache {
	EthernetArpCacheEntry*	table;			///< zero the elements to provide a valid empty table
	int			elements;		///< number of elements
	int			entryLifeTime;		///< how long should the entry be valid? Unit user-defined.
	int			entryRefreshTime;	///< how often should we update used entries? 0 for never.
} EthernetArpCache;

/** Prints out the whole ARP cache.
 * @param outputStream the printing destination
 * @param cache the ARP cache.
 * @param printAll true to print both valid and invalid entries, false to print valid entries only.
 * @return true, if outputStream took all characters, false otherwise.
 */ 
bool fifoPrintEthernetArpCache(Fifo *outputStream, const EthernetArpCache *cache, bool printAll);

/** Looks up the ethernet address of an IP4-address.
 * @param cache the cache.
 * @param ip4Address an IP4 address.
 * @param time the current time.
 * @return the corresponding IP-address if found or 0 if not in cache or expired.
 */
EthernetArpCacheEntry* ethernetArpCacheLookup(EthernetArpCache *cache, const Ip4Address* ip4Address, int time);

/** Finds the least recently set table entry - the most likely to be disposable. Use this function to get new entries
 * even though all entries are in use.
 * @param cache the cache
 * @param ip4Address if !=0 then an entry with this address will be returned if it exists even if its quite new.
 *   Otherwise any other Ip4Address may be in use of the returned entry.
 * @param time the current clock reading.
 * @return a cache entry, not neccessarily expired, not neccessarily unused.
 */
EthernetArpCacheEntry* ethernetArpCacheLru(EthernetArpCache *cache, const Ip4Address* ip4Address, int time);

/** Looks up the next entry in the table that should be actively queried over network.
 * This function is used to extract unresolved or expiring IP-addresses to generate ARP requests that update the cache
 * in response eventually. Unresolved entries will be returned before expiring entries.
 * @param cache the cache.
 * @param time the current time.
 * @return the next unresolved IP address or 0 if no unresolved entries are available.
 */
const Ip4Address* ethernetArpCacheNextRefresh(EthernetArpCache *cache, int time);

/** Delete expired cache entries.
 * @param cache the cache.
 * @param currentTime the current time in us.
 */
void ethernetArpCacheExpire(EthernetArpCache* cache, int currentTime);

/** Set a new entry in the ARP cache. It is guaranteed that the entry will be in the table after the call.
 * If the IP address already existed in the cache, that entry will be updated. If the cache is full, then
 * the least recently used entry will be replaced.
 * @param cache the cache.
 * @param ip4Address guess what?
 * @param ethernetAddress guess what?
 * @param currentTime the clock reading right now.
 */
void ethernetArpCacheSet(EthernetArpCache* cache, const Ip4Address* ip4Address,
	const EthernetAddress* ethernetAddress, int currentTime);


/** This structure is used to reduce the number of parameters of the ARP message handler.
 */
typedef struct {
	const IoFifo			*ioFifo;		///< the packet handler for sending packets
	const EthernetAddress		*ethernetAddress;	///< the local Ethernet address
	const Ip4Address		*ip4Address;		///< the local IP4 address
	EthernetArpCache		*ethernetArpCache;	///< the ARP cache for storing mappings
} EthernetArpIp4Configuration;

/** Sends a packet for resolving the given IP4 address.
 * @param configuration the ARP handler configuration
 * @param address an IP4 address to resolve (to Ethernet address).
 * @return true, if packet was sent, false otherwise (transmission path full).
 */
bool ethernetArpIp4Resolve(const EthernetArpIp4Configuration *configuration, Ip4Address const *address);

/** If there are unresolved or expiring entries in the ARP cache, then one is picked and a request packet is sent.
 * @param configuration the ARP handler configuration
 * @param time user-provided time for calculating timeouts.
 * @return true if a packet was sent, false in case not transmission is possible or no unresolved entries.
 */
bool ethernetArpIp4ResolvePending(const EthernetArpIp4Configuration *configuration, int time);

/** Handles an ARP message. The Ethernet header was decoded and an ARP protocol identifier found and the message
 * body is now handled by this functions.
 * @param configuration the ARP handler configuration
 * @param ethernetHeader the parsed Ethernet header.
 * @param message the ARP message object.
 * @param time user-provided time for calculating timeouts.
 * @return MESSAGE_DONE if message processed successfully, MESSAGE_POSTPONE if send queue is full and MESSAGE_INVALID
 *   if some error occurred.
 */
MessageResult ethernetArpIp4Handle(
	const EthernetArpIp4Configuration	*configuration,
	const EthernetHeader			*ethernetHeader,
	Fifo					*message,
	int					time);

#endif

