void USB_PhyPowerDown_Test()
{
	LDISK_T	 *ptLDisk;
	PDISK_T  *ptPDisk;
	int   	 t_start, t;
	int   	 i, tloop, sec;

	if (get_vdisk('C', &ptLDisk) < 0)
	{
		sysprintf("Disk C not found, USB test aborted!!\n");
		return;
	}
	ptPDisk = ptLDisk->ptPDisk;
	
	t_start = get_timer_ticks();
	
	// 24 hours test loop
	//
	for (tloop = 0; tloop < 24*60*60; tloop++)
	{
		sec = (get_timer_ticks() - t_start) / 100;
		sysprintf("**** Test loop %d elapsed: %02d:%02d:%02d\n", tloop, sec/3600, (sec/60) % 60, sec % 60);
		
		sysprintf("Before suspend: %x\n", inpw(UPSCR1));
				
		outpw(UPSCR1, inpw(UPSCR1) | PORT_SUSPEND);
		for (i = 0; i < 0x10000; i++);
		sysprintf("After set suspend: %x\n", inpw(UPSCR1));
				
		outpw(UCMDR, (inpw(UCMDR) & ~CMD_RUN));
		sysprintf("After clear RUN: %x\n", inpw(UPSCR1));
		sysprintf("After suspend completed: %x\n", inpw(UPSCR1));

		outpw(0xB00050C4, 0xEC);
		outpw(0xB00050C8, 0xEC);
		
		// wait 1 minute
		//
		t = get_timer_ticks();
		while (get_timer_ticks() - t < 6000) ;
		
		outpw(0xB00050C4, 0x160);				
		outpw(0xB00050C8, 0x520);
		
		outpw(UCMDR, (inpw(UCMDR) | CMD_RUN));
		for (i = 0; i < 0x10000; i++);
		sysprintf("After enable RUN: %x\n", inpw(UPSCR1));
				
		outpw(UPSCR1, inpw(UPSCR1) | PORT_RESUME);
		for (i = 0; i < 0x100000; i++);
		sysprintf("After set resume: %x\n", inpw(UPSCR1));
				
		outpw(UPSCR1, inpw(UPSCR1) &~ (PORT_RESUME | PORT_SUSPEND));
		for (i = 0; i < 0x10000; i++);
		sysprintf("After resume completed: %x\n", inpw(UPSCR1));

		// read 128 sectors from disk
		if (ptPDisk->ptDriver->read(ptPDisk, 0, 128, (UINT8 *)_pucDummy) < 0)
		{
			sysprintf("Read disk failed!!\n");
			return;
		}
		else
			sysprintf("Test passed.\n");
	}
}
