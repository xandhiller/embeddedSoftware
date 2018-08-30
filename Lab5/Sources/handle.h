/*! @file
 *
 *  @brief Routines to implement packet handling.
 *
 *  This contains the functions for handling received packets.
 *
 *  @author 12551382 Samin Saif and 11850637 Alex Hiller
 *  @date 2018-05-28
 */

#ifndef HANDLE_H_
#define HANDLE_H_

// PC to Tower commands
#define GET_STARTUP_VALUES  0x04
#define GET_VERSION         0x09
#define TOWER_NUMBER        0x0B
#define TOWER_MODE          0x0D
#define FLASH_PROGRAM_BYTE  0x07
#define FLASH_READ_BYTE     0x08
#define SET_TIME            0x0C
#define PROTOCOL_MODE       0x0A
#define ACCEL_DATA	        0x10

// Tower to PC commands
#define TOWER_STARTUP       0x04
#define TOWER_VERSION       0x09
#define PROTOCOL_MODE	      0x0A

// Accelerometer macros
#define GET_PROTOCOL	    0x01
#define SET_PROTOCOL	    0x02
#define ASYNCHRONOUS	    0x00
#define SYNCHRONOUS	      0x01

// Desired baud rate
static const uint32_t BAUD_RATE = 115200;

/*! @brief Calls the appropriate "Handle" function based on the command received.
 *
 *  @return bool - TRUE if the command received was successfully executed.
 */
void Handle_Packet(void);

/*! @brief Sets the tower number to last 4 digits of student number and stores in flash if tower number has not yet been written.
 *
 *  @return bool - TRUE if the tower number was successfully stored in flash.
 */
bool Initial_TowerNb(void);

/*! @brief Sets the tower mode to 1 and stores in flash if tower mode has not yet been set.
 *
 *  @return bool - TRUE if the tower mode was successfully stored in flash.
 */
bool Initial_TowerMode(void);

/*! @brief Sends the initial 4 packets when tower starts up.
 *
 *  @return bool - TRUE if the 4 packets are all successfully sent.
 */
bool Tower_Startup(void);


#endif /* HANDLE_H_ */
