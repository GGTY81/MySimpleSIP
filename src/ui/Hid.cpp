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

// MySimpleSIP: portado de lib/Hid.cpp do MicroSIP 3.22.16 (fone USB HID: gancho, mudo e
// rediscagem lidos do fone; LEDs de fora do gancho, toque e mudo). Alterações marcadas com
// "MySimpleSIP:". No lugar do hid.c modificado e do hid.dll do Windows: hidapi (hidraw) e
// HidParser (descritor de relatórios). O acesso ao /dev/hidraw* exige a regra do udev
// (scripts/install-hid-rules.sh).

#define THIS_FILENAME "Hid.cpp"

#include "Hid.h"

#include <pjsua-lib/pjsua.h>
#include "global.h"
#include "settings.h"
#include "mainDlg.h"
#include "Dialer.h"

#include <string.h>

// MySimpleSIP: a janela principal da interface (mainDlg é o CmainDlgCore do núcleo)
static CmainDlg* MainDlg()
{
	return static_cast<CmainDlg*>(mainDlg);
}

// MySimpleSIP: adaptação do hidapi ao que o hid.c do MicroSIP oferece ----------------------
namespace {

class HidApiIo : public HidIo
{
public:
	struct hid_device_info *enumerate() override { return hid_enumerate(0x0, 0x0); }
	void free_enumeration(struct hid_device_info *devs) override { hid_free_enumeration(devs); }
	void *open_path(const char *path) override { return hid_open_path(path); }
	int get_report_descriptor(void *dev, unsigned char *buf, size_t size) override
	{
		return hid_get_report_descriptor((hid_device *)dev, buf, size);
	}
	int read(void *dev, unsigned char *data, size_t length) override { return hid_read((hid_device *)dev, data, length); }
	int write(void *dev, const unsigned char *data, size_t length) override
	{
		return hid_write((hid_device *)dev, data, length);
	}
	int set_nonblocking(void *dev, int nonblock) override { return hid_set_nonblocking((hid_device *)dev, nonblock); }
	const wchar_t *error(void *dev) override { return hid_error((hid_device *)dev); }
	void close(void *dev) override { hid_close((hid_device *)dev); }
	void exit() override { hid_exit(); }
};

HidApiIo hidApiIo;
HidIo *io = &hidApiIo;

// hid_open_path do hid.c: abre e prepara o preparsed data da coleção escolhida (no Windows
// cada coleção de nível superior é um dispositivo; no hidraw, uma interface com todas)
msip_hid_device *msip_hid_open_path(const struct hid_device_info *info)
{
	void *handle = io->open_path(info->path);
	if (!handle) {
		return NULL;
	}
	unsigned char desc[HID_API_MAX_REPORT_DESCRIPTOR_SIZE];
	int len = io->get_report_descriptor(handle, desc, sizeof(desc));
	PHIDP_PREPARSED_DATA pp = len > 0 ? HidParseReportDescriptor(desc, len, info->usage_page, info->usage) : NULL;
	if (!pp) {
		io->close(handle);
		return NULL;
	}
	msip_hid_device *dev = new msip_hid_device();
	dev->handle = handle;
	dev->pp_data = pp;
	HidP_GetCaps(pp, &dev->caps);
	dev->report_ids = HidUsesReportIds(pp);
	// cabe qualquer relatório da interface (as outras coleções também chegam pelo hidraw)
	dev->input_report_length = 4096;
	dev->read_buf = new unsigned char[dev->input_report_length];
	return dev;
}

// hid_read do hid.c: o relatório sempre começa pelo report ID (0 se o dispositivo não usa IDs)
int msip_hid_read(msip_hid_device *dev, unsigned char *data, size_t length)
{
	if (dev->report_ids) {
		return io->read(dev->handle, data, length);
	}
	int res = io->read(dev->handle, data + 1, length - 1);
	if (res > 0) {
		data[0] = 0;
		res++;
	}
	return res;
}

// hid_write: sem report IDs o hidapi também espera o 0 na frente, como no Windows
int msip_hid_write(msip_hid_device *dev, const unsigned char *data, size_t length)
{
	return io->write(dev->handle, data, length);
}

int msip_hid_set_nonblocking(msip_hid_device *dev, int nonblock)
{
	return io->set_nonblocking(dev->handle, nonblock);
}

const wchar_t *msip_hid_error(msip_hid_device *dev)
{
	const wchar_t *e = io->error(dev->handle);
	return e ? e : L"";
}

// HidD_FlushQueue: descarta os relatórios já recebidos (leitura não bloqueante)
bool msip_hid_flush(msip_hid_device *dev)
{
	for (int i = 0; i < 64; i++) {
		int res = msip_hid_read(dev, dev->read_buf, dev->input_report_length);
		if (res <= 0) {
			return res == 0;
		}
	}
	return true;
}

void msip_hid_close(msip_hid_device *dev)
{
	io->close(dev->handle);
	HidFreePreparsedData(dev->pp_data);
	delete[] dev->read_buf;
	delete dev;
}

} // namespace

void Hid::SetIo(HidIo *newIo)
{
	io = newIo ? newIo : &hidApiIo;
}
// ------------------------------------------------------------------------------------------

#define HID_USAGE_TELEPHONY_HOOK_SWITCH                 ((USAGE) 0x20)
#define HID_USAGE_TELEPHONY_MUTE                 ((USAGE) 0x2F)
#define HID_USAGE_TELEPHONY_REDIAL                 ((USAGE) 0x24)
#define HID_USAGE_TELEPHONY_FLASH                 ((USAGE) 0x21)
#define HID_USAGE_TELEPHONY_LINE_BUSY                 ((USAGE) 0x97)

msip_hid_device *Hid::m_hDevice = NULL; // MySimpleSIP: dispositivo embrulhado
//bool Hid::m_stateOnline = false;
bool Hid::m_stateOffhook = false;
bool Hid::m_stateRing = false;
bool Hid::m_stateMute = false;

bool m_Offhook = false;
bool m_Redial = false;

UCHAR Hid::m_reportIdOnline;
UCHAR Hid::m_reportIdOffhook;
UCHAR Hid::m_reportIdRing;
UCHAR Hid::m_reportIdMute;

static bool offHookIsAbsolute = false;
static bool muteIsAbsolute = false;
static bool redialIsAbsolute = false;

static bool rejectCall = false;

void CALLBACK Hid::TimerProc(HWND hWnd, UINT nMsg, UINT_PTR nIDEvent, DWORD dwTime)
{
	if (!m_hDevice) {
		return;
	}

	int ReportLength = m_hDevice->input_report_length;
	unsigned char* Report = (unsigned char*)m_hDevice->read_buf;
	ReportLength = msip_hid_read(m_hDevice, Report, ReportLength); // MySimpleSIP

	if (ReportLength > 0) {
		UCHAR ReportId = Report[0];
		if (is_pjsua_running()) {
			PJ_LOG(3, (THIS_FILENAME, "IN --- Received report %d, length %d bytes ---", ReportId, ReportLength));
		}
		USAGE UsagePage = 0x0b;
		USAGE UsageList[32];
		ULONG UsageLength = 32;
		NTSTATUS status = HidP_GetUsages(
			HidP_Input,
			UsagePage,
			0,
			UsageList,
			&UsageLength,
			m_hDevice->pp_data,
			(PCHAR)Report,
			ReportLength
		);
		if (status != HIDP_STATUS_SUCCESS) {
			if (is_pjsua_running()) {
				PJ_LOG(3, (THIS_FILENAME, "HidP_GetUsages failed with code 0x%04hx", status));
			}
		}
		else {
			if (is_pjsua_running()) {
				PJ_LOG(3, (THIS_FILENAME, "IN get %d Usage(s):", UsageLength));
			}
			bool offHook = false;
			bool lineBusy = false;
			bool mute = false;
			bool redial = false;
			for (unsigned int j = 0; j < UsageLength; j++) {
				if (is_pjsua_running()) {
					PJ_LOG(3, (THIS_FILENAME, "IN %02hhx", UsageList[j]));
				}
				switch (UsageList[j]) {
				case HID_USAGE_TELEPHONY_HOOK_SWITCH: // Hook Switch
					offHook = true;
					break;
				case HID_USAGE_TELEPHONY_MUTE: // Phone Mute
					mute = true;
					break;
				case HID_USAGE_TELEPHONY_REDIAL: // Redial
					redial = true;
					break;
				case HID_USAGE_TELEPHONY_FLASH: // Flash (call swap between an active call and a call on hold)
					if (is_pjsua_running()) {
						PJ_LOG(3, (THIS_FILENAME, "IN Event: Flash (not implemented)"));
					}
					break;
				case HID_USAGE_TELEPHONY_LINE_BUSY: // Line Busy (1 = Line is busy / 0 = Line is free)
					lineBusy = true;
					break;
				}
			}
			//-- REDIAL
			// transform from relative to absolute
			if (!redialIsAbsolute) {
				if (!redial) {
					redial = m_Redial;
				}
				else {
					redial = !m_Redial;
				}
			}
			if (redial) {
				if (is_pjsua_running()) {
					PJ_LOG(3, (THIS_FILENAME, "IN Event: Redial"));
				}
                MainDlg()->Redial(); // MySimpleSIP: MainDlg()
			}
			// end transform
			//-- MUTE
			// transform from relative to absolute
			if (!muteIsAbsolute) {
				if (!mute) {
					mute = m_stateMute;
				}
				else {
					mute = !m_stateMute;
				}
			}
			// end transform
			if (m_stateMute != mute) {
				// change host mute state
				MainDlg()->pageDialer->MuteInput(mute); // MySimpleSIP: MainDlg()
				m_stateMute = mute;
			}
			//-- OFF-HOOK
			// transform from relative to absolute
			if (!offHookIsAbsolute) {
				if (!offHook) {
					offHook = m_Offhook;
				}
				else {
					offHook = !m_Offhook;
				}
			}
			// end transform
			if (offHook) {
				if (!m_Offhook) {
					if (!m_stateOffhook) {
						if (m_stateRing) {
							if (is_pjsua_running()) {
								PJ_LOG(3, (THIS_FILENAME, "IN Event: Answer incoming call"));
							}
							MainDlg()->CommandCallAnswer(); // MySimpleSIP: MainDlg()
						}
						else {
							if (is_pjsua_running()) {
								PJ_LOG(3, (THIS_FILENAME, "IN Event: Outgoing call"));
							}
							MainDlg()->pageDialer->Action(ACTION_CALL); // MySimpleSIP: MainDlg()
						}
					}
				}
				else if (m_stateRing) {
					if (rejectCall) {
						if (is_pjsua_running()) {
							PJ_LOG(3, (THIS_FILENAME, "IN Event: Reject call during an active call"));
						}
						MainDlg()->CommandCallReject(); // MySimpleSIP: MainDlg()
					}
					else {
						rejectCall = true;
						return;
					}
				}
			}
			else {
				if (m_Offhook) {
					if (m_stateOffhook) {
						if (is_pjsua_running()) {
							PJ_LOG(3, (THIS_FILENAME, "IN Event: End call"));
						}
                        msip_call_hangup_all_noincoming(true);
					}
				}
				else if (m_stateRing) {
					if (rejectCall) {
						if (is_pjsua_running()) {
							PJ_LOG(3, (THIS_FILENAME, "IN Event: Reject incoming call"));
						}
						MainDlg()->CommandCallReject(); // MySimpleSIP: MainDlg()
					}
					else {
						rejectCall = true;
						return;
					}
				}
			}
			m_Offhook = offHook;
			rejectCall = false;
			//--
		}
	}
	if (ReportLength < 0) {
		if (is_pjsua_running()) {
			PJ_LOG(3, (THIS_FILENAME, "hid_read() returned error"));
		}
		CloseDevice();
	}
}

void Hid::UpdateLED(USAGE Usage, USAGE Usage2)
{
	if (!m_hDevice) {
		return;
	}
	if (is_pjsua_running()) {
		PJ_LOG(3, (THIS_FILENAME, "OUT --- Update LEDs state, Usages: 0x%02hx 0x%02hx ---", Usage, Usage2));
	}
	ULONG ReportLength = m_hDevice->caps.OutputReportByteLength;
	if (is_pjsua_running()) {
		PJ_LOG(3, (THIS_FILENAME, "OUT Report length %d", ReportLength));
	}
	if (ReportLength <= 0) {
		return;
	}
	unsigned char *Report = new (std::nothrow)unsigned char[ReportLength];
	if (Report == nullptr) {
		return;
	}
	USAGE UsagePage = HID_USAGE_PAGE_LED;
	HIDP_BUTTON_CAPS    ButtonCaps[32];
	USHORT              ButtonCapsLength = 32;

	CList <UCHAR, UCHAR> ReportList;
	if (!Usage || Usage == HID_USAGE_LED_ON_LINE || Usage2 == HID_USAGE_LED_ON_LINE) {
		//		if (m_reportIdOnline && !ReportList.Find(m_reportIdOnline)) {
		//			ReportList.AddTail(m_reportIdOnline);
		//		}
	}
	if (!Usage || Usage == HID_USAGE_LED_OFF_HOOK || Usage2 == HID_USAGE_LED_OFF_HOOK) {
		if (m_reportIdOnline && !ReportList.Find(m_reportIdOnline)) {
			ReportList.AddTail(m_reportIdOnline);
		}
		if (m_reportIdOffhook && !ReportList.Find(m_reportIdOffhook)) {
			ReportList.AddTail(m_reportIdOffhook);
		}
	}
	if (!Usage || Usage == HID_USAGE_LED_RING || Usage2 == HID_USAGE_LED_RING) {
		if (m_reportIdRing && !ReportList.Find(m_reportIdRing)) {
			ReportList.AddTail(m_reportIdRing);
		}
	}
	if (!Usage || Usage == HID_USAGE_LED_MUTE || Usage2 == HID_USAGE_LED_MUTE) {
		if (m_reportIdMute && !ReportList.Find(m_reportIdMute)) {
			ReportList.AddTail(m_reportIdMute);
		}
	}

	POSITION pos = ReportList.GetHeadPosition();
	while (pos) {
		POSITION posKey = pos;
		UCHAR ReportID = ReportList.GetNext(pos);
		ZeroMemory(Report, ReportLength);
		Report[0] = ReportID;
		USAGE UsageList[8];
		ULONG UsageLength = 0;
		if (ReportID == m_reportIdOnline) {
			//			if (m_stateOnline) {
			if (m_stateOffhook) {
				UsageList[UsageLength] = HID_USAGE_LED_ON_LINE;
				UsageLength++;
			}
		}
		if (ReportID == m_reportIdOffhook) {
			if (m_stateOffhook) {
				UsageList[UsageLength] = HID_USAGE_LED_OFF_HOOK;
				UsageLength++;
			}
		}
		if (ReportID == m_reportIdRing) {
			if (m_stateRing) {
				UsageList[UsageLength] = HID_USAGE_LED_RING;
				UsageLength++;
			}
		}
		if (ReportID == m_reportIdMute) {
			if (m_stateMute) {
				UsageList[UsageLength] = HID_USAGE_LED_MUTE;
				UsageLength++;
			}
		}
		HidP_SetUsages(
			HidP_Output,
			UsagePage,
			0,
			UsageList,
			&UsageLength,
			m_hDevice->pp_data,
			(PCHAR)Report,
			ReportLength
		);
		if (is_pjsua_running()) {
			PJ_LOG(3, (THIS_FILENAME, "Report:"));
			for (int j = 0; j < ReportLength; j++) {
				PJ_LOG(3, (THIS_FILENAME, "0x%02hx", Report[j]));
			}
			PJ_LOG(3, (THIS_FILENAME, "OUT set %d Usage(s), report id %d", UsageLength, ReportID));
		}
		ULONG BytesWritten = msip_hid_write(m_hDevice, Report, ReportLength); // MySimpleSIP
		if (BytesWritten < 0) {
			if (is_pjsua_running()) {
				PJ_LOG(3, (THIS_FILENAME, "hid_write() returned error %ls", msip_hid_error(m_hDevice)));
			}
			return;
		}
	}
	delete[] Report;
}

void Hid::OpenDevice()
{
	if (m_hDevice) {
		if (is_pjsua_running()) {
			PJ_LOG(3, (THIS_FILENAME, "Device already opened"));
		}
		return;
	}
	struct hid_device_info *devs, *cur_dev, *sel_dev = NULL;
	devs = io->enumerate(); // MySimpleSIP: hidapi trocável
	cur_dev = devs;
	while (cur_dev) {
		if (is_pjsua_running()) {
			PJ_LOG(3, (THIS_FILENAME, "Device Found %04hx:%04hx (usage: %04hx:%04hx)", cur_dev->vendor_id, cur_dev->product_id, cur_dev->usage_page, cur_dev->usage));
			PJ_LOG(3, (THIS_FILENAME, "  Manufacturer: %ls", cur_dev->manufacturer_string));
			PJ_LOG(3, (THIS_FILENAME, "  Product:      %ls", cur_dev->product_string));
		}
		if (!sel_dev && cur_dev->usage_page == 0x0b) {
			sel_dev = cur_dev;
		}
		if (cur_dev->usage_page == 0x0b && (cur_dev->usage == 0x05 || cur_dev->usage == 0x04)) {
			sel_dev = cur_dev;
		}
		cur_dev = cur_dev->next;
	}
	if (!sel_dev) {
		if (is_pjsua_running()) {
			PJ_LOG(3, (THIS_FILENAME, "Telephony Device not found"));
		}
	}
	else {
		if (is_pjsua_running()) {
			PJ_LOG(3, (THIS_FILENAME, "Device Selected %04hx:%04hx (usage: %04hx:%04hx)", sel_dev->vendor_id, sel_dev->product_id, sel_dev->usage_page, sel_dev->usage));
		}
		hid_device *m_hDevice_tmp = NULL;
		m_hDevice = msip_hid_open_path(sel_dev); // MySimpleSIP: com a coleção escolhida
		if (!m_hDevice) {
			if (is_pjsua_running()) {
				PJ_LOG(3, (THIS_FILENAME, "hid_open_path() returned error"));
			}
		}
		else {
			if (is_pjsua_running()) {
				PJ_LOG(3, (THIS_FILENAME, "Device Opened %02hhx %02hhx /iof-rl %d %d %d /nlc %d /ni-bvd %d %d %d /no-bvd %d %d %d /nf-bvd %d %d %d",
					m_hDevice->caps.UsagePage,
					m_hDevice->caps.Usage,
					m_hDevice->caps.InputReportByteLength,
					m_hDevice->caps.OutputReportByteLength,
					m_hDevice->caps.FeatureReportByteLength,
					m_hDevice->caps.NumberLinkCollectionNodes,
					m_hDevice->caps.NumberInputButtonCaps,
					m_hDevice->caps.NumberInputValueCaps,
					m_hDevice->caps.NumberInputDataIndices,
					m_hDevice->caps.NumberOutputButtonCaps,
					m_hDevice->caps.NumberOutputValueCaps,
					m_hDevice->caps.NumberOutputDataIndices,
					m_hDevice->caps.NumberFeatureButtonCaps,
					m_hDevice->caps.NumberFeatureValueCaps,
					m_hDevice->caps.NumberFeatureDataIndices
					));
			}
			// Set the hid_read() function to be non-blocking.
			msip_hid_set_nonblocking(m_hDevice, 1); // MySimpleSIP

			HIDP_BUTTON_CAPS    ButtonCaps[256];
			USHORT              ButtonCapsLength;

			ButtonCapsLength = 256;
			HidP_GetButtonCaps(
				HidP_Input,
				ButtonCaps,
				&ButtonCapsLength,
				m_hDevice->pp_data
			);
			if (is_pjsua_running()) {
				PJ_LOG(3, (THIS_FILENAME, "Input ButtonCaps %d:", ButtonCapsLength));
			}
			for (int i = 0; i < ButtonCapsLength; i++) {
				if (is_pjsua_running()) {
					PJ_LOG(3, (THIS_FILENAME, "%02hhx %d %d %d %d %d %d %02hhx %02hhx %02hhx",
						ButtonCaps[i].UsagePage,
						ButtonCaps[i].ReportID,
						ButtonCaps[i].IsAlias,
						ButtonCaps[i].IsRange,
						ButtonCaps[i].IsStringRange,
						ButtonCaps[i].IsDesignatorRange,
						ButtonCaps[i].IsAbsolute,
						ButtonCaps[i].Range.UsageMin,
						ButtonCaps[i].Range.UsageMax,
						ButtonCaps[i].NotRange.Usage
						));
				}
				if (ButtonCaps[i].UsagePage == HID_USAGE_PAGE_TELEPHONY) {
					if (ButtonCaps[i].NotRange.Usage == HID_USAGE_TELEPHONY_HOOK_SWITCH) {
						offHookIsAbsolute = ButtonCaps[i].IsAbsolute;
					}
					if (ButtonCaps[i].NotRange.Usage == HID_USAGE_TELEPHONY_MUTE) {
						muteIsAbsolute = ButtonCaps[i].IsAbsolute;
					}
					if (ButtonCaps[i].NotRange.Usage == HID_USAGE_TELEPHONY_REDIAL) {
						redialIsAbsolute = ButtonCaps[i].IsAbsolute;
					}
				}
			}

			m_reportIdOnline = 0;
			m_reportIdOffhook = 0;
			m_reportIdRing = 0;

			ButtonCapsLength = 256;
			HidP_GetButtonCaps(
				HidP_Output,
				ButtonCaps,
				&ButtonCapsLength,
				m_hDevice->pp_data
			);
			if (is_pjsua_running()) {
				PJ_LOG(3, (THIS_FILENAME, "Output ButtonCaps %d:", ButtonCapsLength));
			}
			for (int i = 0; i < ButtonCapsLength; i++) {
				if (is_pjsua_running()) {
					PJ_LOG(3, (THIS_FILENAME, "%02hhx %d %d %d %d %d %d %02hhx %02hhx %02hhx",
						ButtonCaps[i].UsagePage,
						ButtonCaps[i].ReportID,
						ButtonCaps[i].IsAlias,
						ButtonCaps[i].IsRange,
						ButtonCaps[i].IsStringRange,
						ButtonCaps[i].IsDesignatorRange,
						ButtonCaps[i].IsAbsolute,
						ButtonCaps[i].Range.UsageMin,
						ButtonCaps[i].Range.UsageMax,
						ButtonCaps[i].NotRange.Usage
						));
				}
				if (ButtonCaps[i].UsagePage == HID_USAGE_PAGE_LED) {
					if (ButtonCaps[i].NotRange.Usage == HID_USAGE_LED_ON_LINE) {
						m_reportIdOnline = ButtonCaps[i].ReportID;
					}
					if (ButtonCaps[i].NotRange.Usage == HID_USAGE_LED_OFF_HOOK) {
						m_reportIdOffhook = ButtonCaps[i].ReportID;
					}
					if (ButtonCaps[i].NotRange.Usage == HID_USAGE_LED_RING) {
						m_reportIdRing = ButtonCaps[i].ReportID;
					}
					if (ButtonCaps[i].NotRange.Usage == HID_USAGE_LED_MUTE) {
						m_reportIdMute = ButtonCaps[i].ReportID;
					}
				}
			}
			if (!msip_hid_flush(m_hDevice)) { // MySimpleSIP: HidD_FlushQueue
				if (is_pjsua_running()) {
					PJ_LOG(3, (THIS_FILENAME, "FlushQueue failed"));
				}
			}
			m_Redial = false;
			m_Offhook = m_stateOffhook;

			UpdateLED();
			mainDlg->SetTimer(IDT_TIMER_HEADSET, 200, &TimerProc);
		}
	}
	io->free_enumeration(devs); // MySimpleSIP: hidapi trocável
}

void Hid::CloseDevice(bool exit)
{
	if (!m_hDevice) {
		return;
	}
	mainDlg->KillTimer(IDT_TIMER_HEADSET);

	if (exit) {
		//		m_stateOnline = false;
		m_stateOffhook = false;
		m_stateRing = false;
		UpdateLED();
	}
	if (is_pjsua_running()) {
		PJ_LOG(3, (THIS_FILENAME, "Device Closed"));
	}

	msip_hid_close(m_hDevice); // MySimpleSIP: libera também o preparsed data
	/* Free static HIDAPI objects. */
	io->exit(); // MySimpleSIP: hidapi trocável
	m_hDevice = NULL;
}

void Hid::SetOnline(bool state)
{
	//	if (m_stateOnline != state) {
	//		if (is_pjsua_running()) {
	//		PJ_LOG(3, (THIS_FILENAME, "Set online: %s", state ? "Yes" : "No"));
	//		}
	//		m_stateOnline = state;
	//		UpdateLED(HID_USAGE_LED_ON_LINE);
	//	}
}

void Hid::SetOffhook(bool state)
{
	if (m_stateOffhook != state) {
		if (is_pjsua_running()) {
			PJ_LOG(3, (THIS_FILENAME, "Set off-hook: %s", state ? "Yes" : "No"));
		}
		m_stateOffhook = state;
		UpdateLED(HID_USAGE_LED_OFF_HOOK);
	}
}

void Hid::SetRing(bool state)
{
	if (m_stateRing != state) {
		if (is_pjsua_running()) {
			PJ_LOG(3, (THIS_FILENAME, "Set ring: %s", state ? "Yes" : "No"));
		}
		m_stateRing = state;
		UpdateLED(HID_USAGE_LED_RING);
	}
}

void Hid::SetOffhookRing(bool stateOffhook, bool stateRing)
{
	if (m_stateOffhook != stateOffhook && m_stateRing != stateRing) {
		if (is_pjsua_running()) {
			PJ_LOG(3, (THIS_FILENAME, "Set off-hook: %s", stateOffhook ? "Yes" : "No"));
			PJ_LOG(3, (THIS_FILENAME, "Set ring: %s", stateRing ? "Yes" : "No"));
		}
		m_stateOffhook = stateOffhook;
		m_stateRing = stateRing;
		UpdateLED(HID_USAGE_LED_OFF_HOOK, HID_USAGE_LED_RING);
	}
	else {
		SetOffhook(stateOffhook);
		SetRing(stateRing);
	}
}

void Hid::SetMute(bool state)
{
	if (m_stateMute != state) {
		if (is_pjsua_running()) {
			PJ_LOG(3, (THIS_FILENAME, "Set mute: %s", state ? "Yes" : "No"));
		}
		m_stateMute = state;
		UpdateLED(HID_USAGE_LED_MUTE);
	}
}
