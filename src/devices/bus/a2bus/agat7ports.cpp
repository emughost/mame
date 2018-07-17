// license:BSD-3-Clause
// copyright-holders:Sergey Svishchev
/*********************************************************************

    agat7_ports.c

    Implementation of the Agat-7 ports card (part number 3.089.106),
    configured as a printer port for Agat-Author text editor.

    To do: other configurations, if any.

    References:
    http://agatcomp.ru/Reading/ebooks-IKP-KPON/KPON.9/V1/AGAWT/SPT:prilojenie2.shtml

*********************************************************************/

#include "emu.h"
#include "agat7ports.h"

//#define VERBOSE 1
#include "logmacro.h"

/***************************************************************************
    PARAMETERS
***************************************************************************/

//**************************************************************************
//  GLOBAL VARIABLES
//**************************************************************************

DEFINE_DEVICE_TYPE(A2BUS_AGAT7_PORTS, a2bus_agat7_ports_device, "a7ports", "Agat-7 Ports Card")

static INPUT_PORTS_START( agat_author )
	PORT_START("PRINTER_CFG")
	PORT_BIT(     0xd9, 0x00, IPT_UNUSED )
	PORT_DIPNAME( 0x02, 0x00, "Printer command set")
	PORT_DIPSETTING(    0x00, "CPA-80/FX-85/Gemini" )
	PORT_DIPSETTING(    0x02, "Mera D100" )
	PORT_DIPNAME( 0x24, 0x00, "Printer character set")
	PORT_DIPSETTING(    0x00, "KOI-8" )
	PORT_DIPSETTING(    0x04, "GOST" )
	PORT_DIPSETTING(    0x20, "CPA-80" )
	PORT_DIPSETTING(    0x24, "FX-85" )
INPUT_PORTS_END

/***************************************************************************
    FUNCTION PROTOTYPES
***************************************************************************/

//-------------------------------------------------
//  device_add_mconfig - add device configuration
//-------------------------------------------------

MACHINE_CONFIG_START(a2bus_agat7_ports_device::device_add_mconfig)
	MCFG_DEVICE_ADD("d9", I8255, 0)
	MCFG_I8255_OUT_PORTA_CB(WRITE8("cent_data_out", output_latch_device, bus_w))
	MCFG_I8255_OUT_PORTB_CB(WRITE8(*this, a2bus_agat7_ports_device, write_portb))
	MCFG_I8255_IN_PORTC_CB(READ8(*this, a2bus_agat7_ports_device, read_portc))

	MCFG_DEVICE_ADD(m_centronics, CENTRONICS, centronics_devices, "printer")
	MCFG_CENTRONICS_BUSY_HANDLER(WRITELINE(*this, a2bus_agat7_ports_device, write_centronics_busy))
	MCFG_CENTRONICS_OUTPUT_LATCH_ADD("cent_data_out", "centronics")

	MCFG_DEVICE_ADD("d10", I8251, 0)
MACHINE_CONFIG_END

//-------------------------------------------------
//  input_ports - device-specific input ports
//-------------------------------------------------

ioport_constructor a2bus_agat7_ports_device::device_input_ports() const
{
	return INPUT_PORTS_NAME(agat_author);
}


//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

a2bus_agat7_ports_device::a2bus_agat7_ports_device(const machine_config &mconfig, device_type type, const char *tag, device_t *owner, uint32_t clock)
	: device_t(mconfig, type, tag, owner, clock)
	, device_a2bus_card_interface(mconfig, *this)
	, m_printer_cfg(*this, "PRINTER_CFG")
	, m_d9(*this, "d9")
	, m_d10(*this, "d10")
	, m_centronics(*this, "centronics")
{
}

a2bus_agat7_ports_device::a2bus_agat7_ports_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock) :
	a2bus_agat7_ports_device(mconfig, A2BUS_AGAT7_PORTS, tag, owner, clock)
{
}

//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void a2bus_agat7_ports_device::device_start()
{
	save_item(NAME(m_centronics_busy));
}

void a2bus_agat7_ports_device::device_reset()
{
	m_centronics_busy = false;
}

/*-------------------------------------------------
    read_c0nx - called for reads from this card's c0nx space
-------------------------------------------------*/

uint8_t a2bus_agat7_ports_device::read_c0nx(uint8_t offset)
{
	u8 data = 0xff;

	switch (offset & 8)
	{
	case 0:
		data = m_d9->read(machine().dummy_space(), offset & 3);
		break;

	case 8:
		if (offset & 1)
		{
			data = m_d10->status_r(machine().dummy_space(), 0);
		}
		else
		{
			data = m_d10->data_r(machine().dummy_space(), 0);
		}
		break;
	}

	return data;
}


/*-------------------------------------------------
    write_c0nx - called for writes to this card's c0nx space
-------------------------------------------------*/

void a2bus_agat7_ports_device::write_c0nx(uint8_t offset, uint8_t data)
{
	switch (offset & 8)
	{
	case 0:
		m_d9->write(machine().dummy_space(), offset & 3, data);
		break;

	case 8:
		if (offset & 1)
		{
			m_d10->control_w(machine().dummy_space(), 0, data);
		}
		else
		{
			m_d10->data_w(machine().dummy_space(), 0, data);
		}
		break;
	}
}

/*
 * 0    ODD
 * 1    EVEN
 * 4    INIT
 * 5    STROBE
 * 6    /INIT
 * 7    /STROBE
 */
WRITE8_MEMBER(a2bus_agat7_ports_device::write_portb)
{
	m_centronics->write_strobe(BIT(data, 5));
	m_centronics->write_init(BIT(data, 4));
}

/*
 * 1    dip CNTRLESC (0: CPA-80, FX-85, Gemini.  1: D100)
 * 2    dip ALF0
 * 3    dip A/BR (0: level, BUSY/READY.  1: edge, ACK)
 * 4    dip INVD (1: data are sent inverted)
 * 5    dip ALF1 (00: KOI-8, 01: GOST, 10: CPA-80, 11: FX-85)
 * 6    dip ABRLEV (0: BUSY, /ACK.  1: READY, ACK)
 * 7    ready signal from device
 */
READ8_MEMBER(a2bus_agat7_ports_device::read_portc)
{
	return (m_centronics_busy << 7) | m_printer_cfg->read();
}

WRITE_LINE_MEMBER(a2bus_agat7_ports_device::write_centronics_busy)
{
	m_centronics_busy = state;
}
