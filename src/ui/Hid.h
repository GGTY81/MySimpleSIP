/* 
 * Copyright (C) 2011-2024 MicroSIP (http://www.microsip.org)
 * Copyright (C) 2026 Giovanni Grimaldi Torelly <giovannigrimaldi@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA 
 */

// MySimpleSIP: portado de lib/Hid.h do MicroSIP 3.22.16. Alterações marcadas com "MySimpleSIP:".

#pragma once
// MySimpleSIP: hidapi do sistema (backend hidraw) e o parser de descritor no lugar do
// "preparsed data" do hid.dll (HidParser.h traz também as constantes do hidusage.h)
#include "afx.h"
#include "HidParser.h"
#include <hidapi/hidapi.h>

// MySimpleSIP: no Windows o hid.c modificado do MicroSIP guarda no hid_device o preparsed
// data, as capacidades e o buffer de leitura; aqui o dispositivo do hidapi é embrulhado
struct msip_hid_device {
	void *handle; // hid_device* (ou o dispositivo falso dos testes)
	PHIDP_PREPARSED_DATA pp_data;
	HIDP_CAPS caps;
	int input_report_length; // com o byte do report ID
	unsigned char *read_buf;
	bool report_ids; // sem report IDs o hidraw não manda o byte 0 da frente
};

// MySimpleSIP: acesso ao hidapi trocável (os testes usam um fone falso)
class HidIo
{
public:
	virtual ~HidIo() = default;
	virtual struct hid_device_info *enumerate() = 0;
	virtual void free_enumeration(struct hid_device_info *devs) = 0;
	virtual void *open_path(const char *path) = 0;
	virtual int get_report_descriptor(void *dev, unsigned char *buf, size_t size) = 0;
	virtual int read(void *dev, unsigned char *data, size_t length) = 0;
	virtual int write(void *dev, const unsigned char *data, size_t length) = 0;
	virtual int set_nonblocking(void *dev, int nonblock) = 0;
	virtual const wchar_t *error(void *dev) = 0;
	virtual void close(void *dev) = 0;
	virtual void exit() = 0;
};

class Hid
{
private:
	static void CALLBACK TimerProc(HWND hWnd,
		UINT nMsg,
		UINT_PTR nIDEvent,
		DWORD dwTime
		);
	static msip_hid_device *m_hDevice; // MySimpleSIP: dispositivo embrulhado

//	static bool m_stateOnline;
	static bool m_stateOffhook;
	static bool m_stateRing;
	static bool m_stateMute;
	static UCHAR m_reportIdOnline;
	static UCHAR m_reportIdOffhook;
	static UCHAR m_reportIdRing;
	static UCHAR m_reportIdMute;
	static void UpdateLED(USAGE Usage = 0, USAGE Usage2 = 0);
public:
	static void OpenDevice();
	static void CloseDevice(bool exit = false);
	static void SetOnline(bool state);
	static void SetOffhook(bool state);
	static void SetRing(bool state);
	static void SetOffhookRing(bool stateOffhook, bool stateRing);
	static void SetMute(bool state);
	// MySimpleSIP: testes
	static void SetIo(HidIo *io); // nullptr = hidapi
	static bool IsOpen() { return m_hDevice != NULL; }
	static void Poll() { TimerProc(NULL, 0, 0, 0); }
};
