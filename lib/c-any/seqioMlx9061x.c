#if 0

bool mlx9061xPrepareRead (Mlx *mlx, Int32 command) {
	seqioReset (&mlx->seqio);
	mlx->crc = crc8Feed (CRCPOLY_8_ITUT, crc8Feed (CRCPOLY_8_ITUT,
		0,(mlx->address & I2C_ADDRESS_MASK)<<1|1),	// Slave address + Wr
		command);		// command byte

	return	seqioPrepareWrite (&mlx->seqio, command, 1, true)
		&& seqioPrepareRead (&mlx->seqio, 3,true)		// 2 bytes + PEC (CRC)
		&& seqioPrepareEnd (&mlx->seqio)
		;
}

/** Call whenever the read transaction finished successfully.
 */
bool mlx9061xFinishRead (Mlx *mlx, Int32 *result) {
	// ASSERTION: seqioIsDone()

	fifoPrintStringLn (fo,"{ MLX90614 finish read }");
	fifoIoWakeUp();
	Uint8 data[3];
	for (int i=0; i<sizeof data; i++)
		if (seqioCanRead (&mlx->seqio)) data[i] = seqioRead (&mlx->seqio);
		else return false;	// failed
		
	const Uint32 myCrc = crc8FeedN (CRCPOLY_8_ITUT,mlx->crc,data,sizeof data);

	if (myCrc==0) {
		*result = data[0] | data[1]<<8;
		return true;
	}
	else {
		fifoPrintString (fo,"CRC mismatch: master=0x");
		fifoPrintHex (fo, myCrc,2,2);
		fifoPrintString (fo,", slave=0x");
		fifoPrintHex (fo, data[2],2,2);
		fifoPrintLn (fo);
		fifoIoWakeUp();
		return false;
	}
}

/** Writes one 16-bit word of data.
 */
bool mlx9061xPrepareWrite (Mlx *mlx, int command, int data) {
	Uint32 crc =
		crc8Feed (CRCPOLY_8_ITUT,
			crc8FeedN (CRCPOLY_8_ITUT,
				crc8Feed (CRCPOLY_8_ITUT,
					crc8Feed (CRCPOLY_8_ITUT, 0,
					mlx->address<<1|1),
				command),
			(Uint8*)&data,2),
		0);				// PEC/CRC placeholder
	return  seqioPrepareWrite (&mlx->seqio, command, 1,false)
		&& seqioPrepareWrite (&mlx->seqio, data, 2,false)
		&& seqioPrepareWrite (&mlx->seqio, crc, 1,false)
		&& seqioPrepareEnd (&mlx->seqio)
		;
}
#endif

