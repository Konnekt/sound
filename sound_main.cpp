//---------------------------------------------------------------------------

#define _WIN32_WINNT 0x0500
#include <windows.h>
#include <stdstring.h>
#include <io.h>
#include <vector>
using namespace std;

#include <Stamina\SimXML.h>
#include <Stamina\WinHelper.h>

#include "konnekt/plug.h"
#include "konnekt/ui.h"
#include "konnekt/plug_export.h"
#include "konnekt/plug_func.h"
#include "sound_main.h"
#include "konnekt/ksound.h"
#include "res\snd\resource.h"
#include "plug_defs/lib.h"


#define SOUND_CFG_NAME "SOUND_"

vector <CStdString> sounds; 
typedef vector <CStdString>::iterator sounds_it; 
bool columnsRegistered = false;
using namespace kSound;
CStdString soundsPath;
CStdString setsPath;
CStdString wavePath;
/*struct cSound {
	unsigned int colID; // ID kolumny z ustawieniem...
	CStdString 
};*/

int WINAPI DllEntryPoint(HINSTANCE hinst, unsigned long reason, void* lpReserved)
{
        return 1;
}
//---------------------------------------------------------------------------
int Init() {
	soundsPath = string((char*)ICMessage(IMC_RESTORECURDIR)) + "sounds";
	setsPath = wavePath = soundsPath;
  return 1;
}

int IStart() {
  return 1;
}
int IEnd() {
  return 1;
}

int ISetCols() {
	SetColumn(DTCFG, kSound::Cfg::mute, DT_CT_INT, 0, "kSound/mute");
    IMessage(kSound::DOREGISTER , NET_BROADCAST);
	columnsRegistered = true;
	sounds.clear(); // nie musimy na razie sprawdzaæ ich poprawnoœci
	return 1;
}

void IUpdate(int ver) {
}

CStdString GetFile(CStdString sndName , int cnt) {
	if (cnt == -1) cnt = 0;
	sndName = SOUND_CFG_NAME + sndName;
	CStdString file;
	int sndID = Ctrl->DTgetNameID(DTCNT , sndName);
	if (sndID == -1) { // Sprawdzamy w konfigu
		sndID = Ctrl->DTgetNameID(DTCFG , sndName);
		file = GETSTR(sndID);
	} else {
	    file = GETCNTC(cnt , sndID);
		if (file.empty() && cnt) file = GETCNTC(0 , sndID);
	}
	file.Trim();
	if (file.empty() || file.size() < 3 || file[0] == '!') return ""; 
	file.Trim();
	if (file[1] != ':' && file[0] != '\\') {
		file = "%KonnektPath%\\sounds\\" + file;
	}
	file = Stamina::expandEnvironmentStrings(file);
    if (_access(file , 4)) return "";
	return file;
}

void Play(const CStdString& sndName , int cnt, bool force = false) {
	if (!force && GETINT(kSound::Cfg::mute))
		return;
	CStdString file = GetFile(sndName, cnt);
	if (file.empty()) return;
    PlaySound(file , 0 , SND_ASYNC | SND_FILENAME | SND_NOWAIT | SND_NOSTOP);
    return;
}

void RegisterAction(kSound::sIMessage_SoundRegister * sr , int parent , int table) {
	UIActionAdd(parent , sr->colID | kSound::action::Check , ACTT_CHECK|ACTSC_INLINE,"",0,14,0 , 0 , table);
	UIActionAdd(parent , sr->colID | kSound::action::Value , ACTT_FILE  | ACTSC_INLINE,"Wybierz dŸwiêk" ,0,120 , 0 , 0 ,table);
	UIActionAdd(parent , sr->colID | kSound::action::Play , ACTT_BUTTON|ACTSC_INLINE,"" AP_IMGURL "res://dll/play.ico" ,0,19 ,19 , 0 , table);
	UIActionAdd(parent , 0 , ACTT_COMMENT , sr->info?sr->info : sr->name);
}

int Register(kSound::sIMessage_SoundRegister * sr) {
	if (!sr->name || !*sr->name) return 0;
	if (find(sounds.begin() , sounds.end() , sr->name) != sounds.end()) return 0;
	CStdString name = SOUND_CFG_NAME + string(sr->name);
	if (!columnsRegistered) {
		// Kolumna do konfiguracji...
		if (!(sr->flags & kSound::flags::no_column_register)) {
			const char* def = (sr->s_size >= sizeof(sIMessage_SoundRegister)) ? sr->defSound : 0;
			sIMessage_setColumn sc((sr->flags & kSound::flags::contacts) ? DTCNT : DTCFG , sr->colID , DT_CT_PCHAR , def , name);
			sr->colID = Ctrl->IMessage(&sc);
		}
	} else {
		if (!(sr->flags & kSound::flags::no_column_register) && sr->colID == -1) {
			sr->colID = Ctrl->DTgetNameID((sr->flags & kSound::flags::contacts) ? DTCNT : DTCFG , name.c_str());
		}
		// Akcja...
		if (!(sr->flags & kSound::flags::no_action_register)) {
			int table = sr->flags & kSound::flags::contacts ? DTCNT : DTCFG;
			RegisterAction(sr , IMIG_SNDCFG_SND , table);
			if (sr->flags & kSound::flags::contacts)
				RegisterAction(sr , IMIG_SNDCNT_SND , table);
		}
	}
	sounds.push_back(sr->name);
	return 1;
}

void startActions(int root , int parent) {
    UIGroupAdd(root , parent,ACTR_INIT,"DŸwiêki" , IDI_SND);
	if (ShowBits::checkBits(ShowBits::showInfoNormal)) {
		if (root == IMIG_NFO) {
			UIActionCfgAddInfoBox(parent, "", "Dla ka¿dego kontaktu z osobna mo¿esz ustawiæ, czy ma byæ odgrywany konkretny dŸwiêk, lub zast¹piæ go innym.", "res://dll/3000.ico", -2, 32);
		} else {
			UIActionCfgAddPluginInfoBox2(parent, "Zestawy dŸwiêków mo¿esz stworzyæ sam, pobraæ ze strony http://www.konnekt.info, lub œci¹gn¹æ z aktualizacji kUpdate."
				"Pobrane zestawy nale¿y umieœciæ w katalogu <i>Konnekt\\sounds</i>."
				, "<br/>(C)2003, 2004 <b>Stamina</b>", "res://dll/3000.ico" AP_TIPRICH_WIDTH "150", -3);
		}
	}

	UIActionAdd(parent , 0 , ACTT_GROUP , "Zestawy dŸwiêków"); {
		UIActionAdd(parent , IMIA_SNDCFG_LOAD , ACTT_BUTTON|ACTSC_INLINE | ACTSC_BOLD , "Wczytaj zestaw" AP_IMGURL "res://ui/import.ico" AP_ICONSIZE "32" , 0 , 150 , 42);
		UIActionAdd(parent , IMIA_SNDCFG_GOTODIR , ACTT_BUTTON , "Eksploruj" AP_IMGURL "res://ui/search.ico" AP_ICONSIZE "32" , 0 , 120 , 42);
	}UIActionAdd(parent , 0 , ACTT_GROUPEND);

    UIActionAdd(parent , 0 , ACTT_GROUP , "DŸwiêki");

}
void endActions(int root , int parent) {
    UIActionAdd(parent , 0 , ACTT_GROUPEND);
}

int IPrepare() {
  // Ustawienia domyœlne dla pierwszego kontaktu.

	ICMessage(IMI_ICONRES,IDI_SND);
	ICMessage(IMI_ICONRES,IDI_SNDMUTE);
	startActions(IMIG_CFG_UI , IMIG_SNDCFG_SND);
	startActions(IMIG_NFO , IMIG_SNDCNT_SND);
	IMessage(kSound::DOREGISTER , NET_BROADCAST);
	endActions(IMIG_CFG_UI , IMIG_SNDCFG_SND);
	endActions(IMIG_NFO , IMIG_SNDCNT_SND);

	UIActionAdd(IMIG_SNDCFG_SND , 0 , ACTT_GROUP , "");{
		UIActionAdd(IMIG_SNDCFG_SND , 0 , ACTT_CHECK , "Wycisz wszystkie dŸwiêki", kSound::Cfg::mute);
	}UIActionAdd(IMIG_SNDCFG_SND , 0 , ACTT_GROUPEND);

	UIActionAdd(ICMessage(IMI_GETPLUGINSGROUP), kSound::action::mute, ACTT_CHECK | (GETINT(Cfg::mute)?ACTS_CHECKED:0), "Wycisz", IDI_SNDMUTE);

  return 1;
}


void loadSet(sUIAction act) {
    static CStdString fileName="default.xml";
    OPENFILENAME of;
    memset(&of , 0 , sizeof(of));
    of.lpstrTitle = (char*)"Wybierz zestaw dŸwiêkowy";
    sUIActionInfo ai;
    ai.act = act;
    ICMessage(IMI_ACTION_GET , (int)&ai);
    of.hwndOwner = (HWND)ai.handle;
    of.lStructSize=sizeof(of)-12;
    of.lpstrFilter="Zestaw dŸwiêkowy (*.xml)\0*.xml\0";
    of.lpstrFile=fileName.GetBuffer(255);
    of.nMaxFile=255;
    of.Flags=OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
    of.lpstrDefExt="";

/*	sMRU mru;
	CStdString mru_result, dir;
	mru.name = "kSound_dir";
	mru.flags = MRU_GET_ONEBUFF;
	mru.buffer = mru_result.GetBuffer(255);
	mru.buffSize = 255;
	mru.count = 1;
	Ctrl->IMessage(&sIMessage_MRU(IMC_MRU_GET , &mru));
	mru_result.ReleaseBuffer();
	dir = mru.values[0];
	if (dir.empty()) {
		dir = soundsPath;
	}*/
	of.lpstrInitialDir = setsPath;

    //of.lpstrInitialDir = "sounds\\";
    if (GetOpenFileName(&of)) {
		{
			setsPath = of.lpstrFile;
			if (setsPath.find('\\')!=-1) setsPath.erase(setsPath.find_last_of('\\'));
			/*mru.current = dir;
			Ctrl->IMessage(&sIMessage_MRU(IMC_MRU_SET , &mru));*/
		}
		fileName.ReleaseBuffer();
		Stamina::SXML xml;
        xml.loadFile(fileName.c_str());
        CStdString dir = fileName;
        dir.erase(dir.find_last_of('\\')+1);
		dir.MakeLower();
		dir.Replace((CStdString((char*) ICMessage(IMC_RESTORECURDIR)) + "sounds\\").ToLower() , "");
		for (sounds_it i = sounds.begin(); i != sounds.end(); i++) {
			act.id = Ctrl->DTgetNameID(DTCNT , (SOUND_CFG_NAME + *i));
			if (act.id == -1) act.id = Ctrl->DTgetNameID(DTCFG , (SOUND_CFG_NAME + *i));
			if (act.id == -1) continue;
			act.id |= kSound::action::Value;
			CStdString value = xml.getText("sound/events/"+*i);
			if (!value.empty())
				UIActionCfgSetValue(act , (dir+value).c_str());
		}
        string info = xml.getText("sound/name") + "\r\n";
        info+= "Autor: " + xml.getText("sound/author") + "\r\n";
        info+= xml.getText("sound/url") + "\r\n\r\n";
        info+= "" + xml.getText("sound/comment") + "\r\n";

        MessageBox(0 , info.c_str() , "Informacje o zestawie dŸwiêkowym" , 0);
             
    } else {
        int error = CommDlgExtendedError();
        fileName.ReleaseBuffer();}

    
}



int ActionSndValue(sUIActionNotify_base * anBase) {
    sUIActionNotify_2params * an = static_cast<sUIActionNotify_2params*>(anBase);
    switch (an->code) {
        case ACTN_FILEOPEN: {
            OPENFILENAME * ofn = (OPENFILENAME*)an->notify1;
            ofn->lpstrFilter = "WaveFile (*.wav)\0*.wav\0Wszystkie typy (*.*)\0*.*\0\0";
			ofn->lpstrInitialDir = wavePath;
			CStdString file = ofn->lpstrFile;
			if (file.length() && (file.find(':')==-1 || file[0]!='\\')) {
				file = setsPath + "\\" + file;
				strcpy((char*)ofn->lpstrFile , file);
			}
            return 1;}
        case ACTN_FILEOPENED: {
            OPENFILENAME * ofn = (OPENFILENAME*)an->notify1;
			CStdString file = ofn->lpstrFile;
			wavePath = file.ToLower();
			if (wavePath.find('\\')!=-1) wavePath.erase(wavePath.find_last_of('\\'));
			if (file.ToLower().find(setsPath.ToLower()) == 0) {
				file.erase(0, setsPath.size() + 1);
				strcpy((char*)ofn->lpstrFile , file);
			}
			return 0;}
        case ACTN_GET: {
            char * val = (char*)an->notify1;
			sUIActionInfo si(an->act);
			si.mask = UIAIM_PARAM;
			UIActionGet(si);
			int cnt = an->act.cnt;
			if (cnt < 0) cnt = 0;
			strncpy(val , Ctrl->DTgetStr(si.param , cnt , an->act.id & (~IMIB_)) , an->notify2);
            if (*val=='!') an->notify1++;
			UIActionCfgSetValue(sUIAction(an->act.parent , an->act.id & (~IMIB_) | kSound::action::Check , an->act.cnt) , *val=='!'?"0":"1");
            return 0;}
        case ACTN_SET: {
            std::string val = (char*)an->notify1;
            if (*UIActionCfgGetValue(sUIAction(an->act.parent , an->act.id & (~IMIB_) | kSound::action::Check , an->act.cnt) , 0 , 0)!='1') {
                val='!'+val;
            }
			sUIActionInfo si(an->act);
			si.mask = UIAIM_PARAM;
			UIActionGet(si);
			int cnt = an->act.cnt;
			if (cnt < 0) cnt = 0;
			Ctrl->DTsetStr(si.param , cnt , an->act.id & (~IMIB_) , val.c_str());
            return 0;}
    }
    return 0;
}

int ActionSndCheck(sUIActionNotify_base * anBase) {
  sUIActionNotify_2params * an = static_cast<sUIActionNotify_2params*>(anBase);
  switch (an->act.id & ~IMIB_) {
    
  }
  return 0;
}

int ActionSndPlay(sUIActionNotify_base * anBase) {
  sUIActionNotify_2params * an = static_cast<sUIActionNotify_2params*>(anBase);
  ACTIONONLY(an);
  static int playing=0;
  if (playing != an->act.id) {
      CStdString snd;
	  UIActionCfgGetValue(sUIAction(an->act.parent , (an->act.id & ~IMIB_)|kSound::action::Value , an->act.cnt), snd.GetBuffer(255) , 255);
      snd.ReleaseBuffer();
      playing = an->act.id;
	  if (snd.size()<2) {
		  if (an->act.cnt > 0) {
 			  CStdString name = SAFECHAR(Ctrl->DTgetName(DTCNT, an->act.id & ~IMIB_));
			  name.erase(0, strlen(SOUND_CFG_NAME));
			  if (!name.empty()) {
				  Play(name, an->act.cnt, true);
			  }
		  }
		  return 0;
	  }
      if (snd[1]!=':') snd="sounds\\"+snd;
      PlaySound(snd.c_str() , 0 , SND_ASYNC | SND_FILENAME | SND_NOWAIT);
  } else {PlaySound(0,0,0);playing=0;}
  return 0;
}


ActionProc(sUIActionNotify_base * anBase) {
  sUIActionNotify_2params * an = static_cast<sUIActionNotify_2params*>(anBase);
  if ((an->act.id & IMIB_) == kSound::action::Value) return ActionSndValue(an); 
  if ((an->act.id & IMIB_) == kSound::action::Check) return ActionSndCheck(an); 
  if ((an->act.id & IMIB_) == kSound::action::Play) return ActionSndPlay(an); 

  switch (an->act.id) {
      case IMIG_SNDCNT_SND: 
          if (an->code == ACTN_CREATE) {
              UIActionSetStatus(an->act , Ctrl->DTgetPos(DTCNT , an->act.cnt)?0:ACTS_HIDDEN , ACTS_HIDDEN);
          }
          return 0;
      case IMIA_SNDCFG_LOAD:
          ACTIONONLY(an);
          loadSet(an->act);
          return 0;
	  case IMIA_SNDCFG_GOTODIR:
		  ACTIONONLY(an);
		  ShellExecute(0, "explore", soundsPath, "", "", SW_SHOW);			
		  return 0;
	  case action::mute:
		  SETINT(Cfg::mute, !GETINT(Cfg::mute));
		  IMessageDirect(IM_CFG_CHANGED);
		  return 0;
  }
  return 0;
}




int __stdcall IMessageProc(sIMessage_base * msgBase) {
 sIMessage_2params * msg = static_cast<sIMessage_2params*>(msgBase);
 //int a , r;
 cMessage * m;
 switch (msg->id) {
    // INITIALIZATION
    case IM_PLUG_NET:        return NET_SOUND;
    case IM_PLUG_TYPE:       return IMT_MESSAGE | IMT_CONFIG | IMT_ALLMESSAGES | IMT_CONTACT | IMT_UI;
    case IM_PLUG_VERSION:    return (int)"";
    case IM_PLUG_SDKVERSION: return KONNEKT_SDK_V;  // Ta linijka jest wymagana!
    case IM_PLUG_SIG:        return (int)"SOUND";
    case IM_PLUG_CORE_V:     return (int)"W98";
    case IM_PLUG_UI_V:       return 0;
    case IM_PLUG_NAME:       return (int)"Sound Events";
    case IM_PLUG_NETNAME:    return (int)"";
    case IM_PLUG_INIT:       Ctrl=(cCtrl*)msg->p1;Plug_Init(msg->p1,msg->p2);return Init();
    case IM_PLUG_DEINIT:     Plug_Deinit(msg->p1,msg->p2);return 1;
	case IM_PLUG_PRIORITY:   return PLUGP_HIGH;

    case IM_SETCOLS:     return ISetCols();

    case IM_UI_PREPARE:      return IPrepare();
    case IM_START:           return IStart();
    case IM_END:             return IEnd();

    case IM_UIACTION:        return ActionProc((sUIActionNotify_base*)msg->p1);

	case IM_CNT_STATUSCHANGE: {
//         IMLOG("STATUS change");
		sIMessage_StatusChange * sc = static_cast<sIMessage_StatusChange*>(msgBase);
		if (sc->status != -1 
			&& (GETCNTI(sc->cntID , CNT_STATUS)&ST_MASK) < ST_SEMIONLINE 
			&& sc->status >= ST_ONLINE
			)
            Play("newUser" , msg->p1);
		return 0;}
	case IM_CFG_CHANGED:
	  UIActionSetStatus(sUIAction(ICMessage(IMI_GETPLUGINSGROUP), action::mute), (GETINT(Cfg::mute)?-1:0), ACTS_CHECKED);
	  break;


    case IM_PLUG_UPDATE:
        IUpdate(msg->p1);
        return 1;
    case IM_MSG_RCV:
            { m = (cMessage*)msg->p1;
              int cnt = ICMessage(IMC_CNT_FIND , (int)m->net , (int)(m->flag&MF_SEND?m->toUid:m->fromUid));
			  if (!GetExtParam(m->ext, MEX_NOSOUND).empty())
				  return 0;
              switch (m->type) {
                 case MT_SERVEREVENT:
                   Play("newServerEvent" , cnt);
                   return 0;
                 case MT_QUICKEVENT:
                   Play("quickEvent" , cnt);
                   return 0;
                 case MT_MESSAGE:
                   if (m->flag & MF_SEND) {
                     if (ICMessage(IMI_MSG_WINDOWSTATE , cnt))
                       Play("msgSent" , cnt);
                     return 0;}
                   switch (ICMessage(IMI_MSG_WINDOWSTATE , cnt)) {
					 case 0: Play("newMsg",cnt);break;
                     case 1: Play("newMsgAct" , cnt);break;
					 case -1:case -2: Play("newMsgInact" , cnt);break;
                   }
                   return 0;
              }
              return 0;
            }

	case kSound::PLAY:
		if (!msg->p1) return 0;
		Play((char*)msg->p1 , msg->p2);
		return 1;
	case kSound::GETFILE: {
		if (!msg->p1) return 0;
		CStdString file = GetFile((char*)msg->p1 , msg->p2);
		if (file.empty())
			return 0;
		char* buff = (char*) Ctrl->GetTempBuffer(file.size() + 1);
		strcpy(buff, file.c_str());
		return (int)buff;}
	case kSound::REGISTER:
		if (msgBase->s_size < sIMessage_SoundRegister::minimumSize ) return 0;
		return Register(static_cast<kSound::sIMessage_SoundRegister*>(msgBase));
	case kSound::DOREGISTER:
		Register(&kSound::sIMessage_SoundRegister("newUser" , "Ktoœ jest dostêpny." , kSound::flags::contacts , CNT_SND_NEWUSER));
		Register(&kSound::sIMessage_SoundRegister("newMsg" , "Wiadomoœæ (zamkniête okno)" , kSound::flags::contacts , CNT_SND_NEWMSG));
		Register(&kSound::sIMessage_SoundRegister("newMsgAct" , "Wiadomoœæ , aktywne okno" , kSound::flags::contacts , CNT_SND_NEWMSGA));
		Register(&kSound::sIMessage_SoundRegister("newMsgInact" , "Wiadomoœæ , nieaktywne okno rozmowy" , kSound::flags::contacts , CNT_SND_NEWMSGINA));
		Register(&kSound::sIMessage_SoundRegister("newServerEvent" , "Wiadomoœæ od serwera" , 0 , CNT_SND_NEWSERVEREVENT));
		Register(&kSound::sIMessage_SoundRegister("quickEvent" , "Zdarzenie (np. niedostarczenie wiadomoœci)" , kSound::flags::contacts , CNT_SND_QUICKEVENT));
		Register(&kSound::sIMessage_SoundRegister("msgSent" , "Wiadomoœæ wys³ana" , kSound::flags::contacts , CNT_SND_MSGSENT));
		return 0;
    default:
        if (Ctrl) Ctrl->setError(IMERROR_NORESULT);
        return 0;

 }
 if (Ctrl) Ctrl->setError(IMERROR_NORESULT);
 return 0;
}

