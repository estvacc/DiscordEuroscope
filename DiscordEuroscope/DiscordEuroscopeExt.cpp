#include "stdafx.h"
#include "DiscordEuroscopeExt.h"


DiscordEuroscopeExt::DiscordEuroscopeExt() : EuroScopePlugIn::CPlugIn(EuroScopePlugIn::COMPATIBILITY_CODE, "Discord Euroscope for VACC Estonia", "1.2.0", "Mark-Julius Pikat", "https://www.github.com/estvacc/")
{
	DiscordEventHandlers handlers;
	memset(&handlers, 0, sizeof(handlers));
	// handlers
	Discord_Initialize("1138560362531258460", &handlers, 1, NULL);
	this->EuroInittime = (int)time(NULL);

	char DllPathFile[_MAX_PATH];
	std::string RCPath;
	GetModuleFileNameA(HINSTANCE(&__ImageBase), DllPathFile, sizeof(DllPathFile));
	RCPath = DllPathFile;
	RCPath.resize(RCPath.size() - strlen("DiscordEuroscope.dll"));
	RCPath += "DiscordEuroscope_RadioCallsigns.txt";

	fclose(fopen(RCPath.c_str(), "a")); // Creates the file if not exists

	std::ifstream fs;
	fs.open(RCPath.c_str(), std::ifstream::in);

	while (fs.good())
	{
		char* line = new char[50];
		fs.getline(line, 50);
		if (line != 0)
		{
			// I want to split in C instead
			if (std::string(line).find(' ') == std::string::npos)
			{delete line; continue;}
			std::string callsign;
			std::string radioname;
			int i = 0;
			while (*(line + i) != ' ')
			{
				callsign += (*(line + i));
				i++;
			}
			i++; // space before radioname
			while (*(line + i) != 0)
			{
				radioname += (*(line + i));
				i++;
			}
			if (callsign.length() > 0 && radioname.length() > 0)
			{
				RadioCallsigns.insert(std::pair<std::string, std::string>(callsign, radioname));
			}
		}
		delete line;
	}
	if(fs.is_open())
		fs.close();
#ifdef EUROSCOPE32
	char* dmsg = new char[100];
	int count = RadioCallsigns.size();
	sprintf(dmsg, "Successfully parsed %i callsign%s", count, count == 1 ? "!" : "s!");
	DisplayUserMessage("Message", "DiscordEuroscope", dmsg, true, true, false, true, false);
	if (count == 0)
	{
		DisplayUserMessage("Message", "DiscordEuroscope", "If you haven't configured this properly, make sure you are writing to", true, true, false, true, false);
		DisplayUserMessage("Message", "DiscordEuroscope", "DiscordEuroscope_RadioCallsigns.txt, each line holds a callsign", true, true, false, true, false);
		DisplayUserMessage("Message", "DiscordEuroscope", "Example: EETT_CTR Tallinn Control", true, true, false, true, false);
	}
	delete dmsg;
#endif
}


DiscordEuroscopeExt::~DiscordEuroscopeExt()
{
	RadioCallsigns.clear();
}

VOID CALLBACK DiscordTimer(_In_ HWND hwnd, _In_ UINT uMsg, _In_ UINT_PTR idEvent, _In_ DWORD dwTime)
{
	if (inst == nullptr)
		return;
	if (inst->EuroInittime == 0)
		return;
	DiscordRichPresence discordPresence;
	memset(&discordPresence, 0, sizeof(discordPresence));
	discordPresence.largeImageKey = "esto_tower_white";
	discordPresence.startTimestamp = inst->EuroInittime;
	switch (pMyPlugIn->GetConnectionType())
	{
		using namespace EuroScopePlugIn;
	case CONNECTION_TYPE_NO:
		discordPresence.details = "Idle";
		Discord_UpdatePresence(&discordPresence);
		return;
	case CONNECTION_TYPE_PLAYBACK:
		discordPresence.details = "Playback";
		
		Discord_UpdatePresence(&discordPresence);
		return;
	case CONNECTION_TYPE_SWEATBOX:
		discordPresence.details = "Sweatbox";
		Discord_UpdatePresence(&discordPresence);
		return;
	case CONNECTION_TYPE_DIRECT:
		break;
	default:
		Discord_ClearPresence();
		return;
	}
	EuroScopePlugIn::CController controller = pMyPlugIn->ControllerMyself();
	const char* callsign = controller.GetCallsign();
	double frequency = controller.GetPrimaryFrequency();
	char* tmp = new char[100];
	char* tmp2 = new char[100];
	char* tmp3 = new char[100];
	if (controller.IsController())
	{
		if (inst->RadioCallsigns.find(callsign) != inst->RadioCallsigns.end())
			discordPresence.largeImageText = inst->RadioCallsigns[callsign].c_str();
		sprintf(tmp, "%s %.3f MHz", callsign, frequency);
		sprintf(tmp2, "Aircraft Tracked (%i of %i)", inst->CountTrackedAC(), inst->CountACinRange());
		if (inst->tracklist.size() > 0) {
			sprintf(tmp3, "Total Aircraft: %i", inst->tracklist.size());
			discordPresence.smallImageText = tmp3;
			discordPresence.smallImageKey = "esto_ttrks";
		}
	}
	else
	{
		sprintf(tmp, "Observing as %s", callsign);
		sprintf(tmp2, "Aircraft Seen: %i", inst->CountACinRange());
	}
	discordPresence.details = tmp;
	discordPresence.state = tmp2;
	discordPresence.startTimestamp = inst->EuroInittime;
	Discord_UpdatePresence(&discordPresence);
	Discord_RunCallbacks();
	delete tmp;
	delete tmp2;
	delete tmp3;
}


int DiscordEuroscopeExt::CountACinRange()
{
	int count = 0;
	EuroScopePlugIn::CRadarTarget ac = RadarTargetSelectFirst();

	while (ac.IsValid())
	{
		count++;
		ac = RadarTargetSelectNext(ac);
	}
	return count;
}

int DiscordEuroscopeExt::CountTrackedAC()
{
	int count = 0;
	EuroScopePlugIn::CRadarTarget ac = RadarTargetSelectFirst();

	while (ac.IsValid())
	{
		if (ac.GetCorrelatedFlightPlan().GetTrackingControllerIsMe() == true)
		{
			count++;
			if (std::find(inst->tracklist.begin(), inst->tracklist.end(), ac.GetCallsign()) == inst->tracklist.end())
				inst->tracklist.push_back(ac.GetCallsign());
		}
		ac = RadarTargetSelectNext(ac);
	}
	return count;
}