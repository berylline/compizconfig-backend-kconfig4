/*
 *  KDE4 libcompizconfig backend
 *
 *  Copyright (c) 2008 Dennis Kasprzyk <onestone@compiz-fusion.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

#include <QCoreApplication>
#include <QFile>
#include <QDir>
#include <QList>

#include <KConfig>
#include <KConfigGroup>
#include <KGlobal>
#include <KStandardDirs>
#include <KComponentData>
#include <KDebug>
#include <KShortcut>

#include "kwin_interface.h"

#include <stdlib.h>
#include <X11/X.h>
#include <X11/Xlib.h>

extern "C"
{
#include <ccs.h>
#include <ccs-backend.h>
}

#define CORE_NAME "core"

#define CompAltMask        (1 << 16)
#define CompMetaMask       (1 << 17)
#define CompSuperMask      (1 << 18)
#define CompHyperMask      (1 << 19)
#define CompModeSwitchMask (1 << 20)
#define CompNumLockMask    (1 << 21)
#define CompScrollLockMask (1 << 22)

typedef struct _ConfigFiles
{
    QString        profile;

    KConfig        *main;
    KConfig        *kwin;
    KConfig        *shortcuts;

    Bool           modified;
    unsigned int   mainWatch;
    unsigned int   kwinWatch;
    unsigned int   shortcutWatch;
}
ConfigFiles;

static ConfigFiles *cFiles = NULL;

typedef enum
{
    OptionInt,
    OptionBool,
    OptionKey,
    OptionSpecial
}
SpecialOptionType;

struct _SpecialOption
{
    QString           settingName;
    QString           pluginName;
    QString           kdeName;
    QString           groupName;
    SpecialOptionType type;
}

const specialOptions[] =
{
    {"close_window_key", CORE_NAME, "Window Close", "kwin", OptionKey},
    {"lower_window_key", CORE_NAME, "Window Lower", "kwin", OptionKey},
    {"toggle_window_maximized_key", CORE_NAME, "Window Maximize", "kwin", OptionKey},
    {"minimize_window_key", CORE_NAME, "Window Minimize", "kwin", OptionKey},
    {"toggle_window_maximized_horizontally_key", CORE_NAME, "Window Maximize Horizontal", "kwin", OptionKey},
    {"toggle_window_maximized_vertically_key", CORE_NAME, "Window Maximize Vertical", "kwin", OptionKey},
    {"window_menu_key", CORE_NAME, "Window Operations Menu", "kwin", OptionKey},
    
    {"toggle_window_shaded_key", CORE_NAME, "Window Shade", "kwin", OptionKey},
    {"raise_window_key", CORE_NAME, "Window Raise", "kwin", OptionKey},
    {"toggle_window_fullscreen_key", CORE_NAME, "Window Fullscreen", "kwin", OptionKey},
    {"run_command11_key", "commands", "Kill Window", "kwin", OptionKey},
    {"initiate_key", "move", "Window Move", "kwin", OptionKey},
    {"initiate_key", "resize", "Window Resize", "kwin", OptionKey},
    {"rotate_right_key", "rotate", "Switch to Next Desktop", "kwin", OptionKey},
    {"rotate_left_key", "rotate", "Switch to Previous Desktop", "kwin", OptionKey},
    {"rotate_to_1_key", "rotate", "Switch to Desktop 1", "kwin", OptionKey},
    {"rotate_to_2_key", "rotate", "Switch to Desktop 2", "kwin", OptionKey},
    {"rotate_to_3_key", "rotate", "Switch to Desktop 3", "kwin", OptionKey},
    {"rotate_to_4_key", "rotate", "Switch to Desktop 4", "kwin", OptionKey},
    {"rotate_to_5_key", "rotate", "Switch to Desktop 5", "kwin", OptionKey},
    {"rotate_to_6_key", "rotate", "Switch to Desktop 6", "kwin", OptionKey},
    {"rotate_to_7_key", "rotate", "Switch to Desktop 7", "kwin", OptionKey},
    {"rotate_to_8_key", "rotate", "Switch to Desktop 8", "kwin", OptionKey},
    {"rotate_to_9_key", "rotate", "Switch to Desktop 9", "kwin", OptionKey},
    {"rotate_to_10_key", "rotate", "Switch to Desktop 10", "kwin", OptionKey},
    {"rotate_to_11_key", "rotate", "Switch to Desktop 11", "kwin", OptionKey},
    {"rotate_to_12_key", "rotate", "Switch to Desktop 12", "kwin", OptionKey},
    {"rotate_right_window_key", "rotate", "Window to Next Desktop", "kwin", OptionKey},
    {"rotate_left_window_key", "rotate", "Window to Previous Desktop", "kwin", OptionKey},
    {"rotate_to_1_window_key", "rotate", "Window to Desktop 1", "kwin", OptionKey},
    {"rotate_to_2_window_key", "rotate", "Window to Desktop 2", "kwin", OptionKey},
    {"rotate_to_3_window_key", "rotate", "Window to Desktop 3", "kwin", OptionKey},
    {"rotate_to_4_window_key", "rotate", "Window to Desktop 4", "kwin", OptionKey},
    {"rotate_to_5_window_key", "rotate", "Window to Desktop 5", "kwin", OptionKey},
    {"rotate_to_6_window_key", "rotate", "Window to Desktop 6", "kwin", OptionKey},
    {"rotate_to_7_window_key", "rotate", "Window to Desktop 7", "kwin", OptionKey},
    {"rotate_to_8_window_key", "rotate", "Window to Desktop 8", "kwin", OptionKey},
    {"rotate_to_9_window_key", "rotate", "Window to Desktop 9", "kwin", OptionKey},
    {"rotate_to_10_window_key", "rotate", "Window to Desktop 10", "kwin", OptionKey},
    {"rotate_to_11_window_key", "rotate", "Window to Desktop 11", "kwin", OptionKey},
    {"rotate_to_12_window_key", "rotate", "Window to Desktop 12", "kwin", OptionKey},

    {"next_key", "wall", "Switch to Next Desktop", "kwin", OptionKey},
    {"prev_key", "wall", "Switch to Previous Desktop", "kwin", OptionKey},
    {"right_window_key", "wall", "Window One Desktop to the Right", "kwin", OptionKey},
    {"left_window_key", "wall", "Window One Desktop to the Left", "kwin", OptionKey},
    {"up_window_key", "wall", "Window One Desktop Up", "kwin", OptionKey},
    {"down_window_key", "wall", "Window One Desktop Down", "kwin", OptionKey},
    {"up_key", "wall", "Switch One Desktop Up", "kwin", OptionKey},
    {"down_key", "wall", "Switch One Desktop Down", "kwin", OptionKey},
    {"left_key", "wall", "Switch One Desktop to the Left", "kwin", OptionKey},
    {"right_key", "wall", "Switch One Desktop to the Right", "kwin", OptionKey},

    {"switch_to_1_key", "vpswitch", "Switch to Desktop 1", "kwin", OptionKey},
    {"switch_to_2_key", "vpswitch", "Switch to Desktop 2", "kwin", OptionKey},
    {"switch_to_3_key", "vpswitch", "Switch to Desktop 3", "kwin", OptionKey},
    {"switch_to_4_key", "vpswitch", "Switch to Desktop 4", "kwin", OptionKey},
    {"switch_to_5_key", "vpswitch", "Switch to Desktop 5", "kwin", OptionKey},
    {"switch_to_6_key", "vpswitch", "Switch to Desktop 6", "kwin", OptionKey},
    {"switch_to_7_key", "vpswitch", "Switch to Desktop 7", "kwin", OptionKey},
    {"switch_to_8_key", "vpswitch", "Switch to Desktop 8", "kwin", OptionKey},
    {"switch_to_9_key", "vpswitch", "Switch to Desktop 9", "kwin", OptionKey},
    {"switch_to_10_key", "vpswitch", "Switch to Desktop 10", "kwin", OptionKey},
    {"switch_to_11_key", "vpswitch", "Switch to Desktop 11", "kwin", OptionKey},
    {"switch_to_12_key", "vpswitch", "Switch to Desktop 12", "kwin", OptionKey},

    {"initiate_key", "scale", "Expose", "kwin", OptionKey},
    {"initiate_all_key", "scale", "ExposeAll", "kwin", OptionKey},
    {"expo_key", "expo", "ShowDesktopGrid", "kwin", OptionKey},

    {"autoraise", CORE_NAME, "AutoRaise", "Windows", OptionBool},
    {"raise_on_click", CORE_NAME, "ClickRaise", "Windows", OptionBool},
    {"snapoff_maximized", "move", "MoveResizeMaximizedWindows", "Windows", OptionBool},
    {"always_show", "resizeinfo", "GeometryTip", "Windows", OptionBool},
    {"allow_wraparound", "wall", "RollOverDesktops", "Windows", OptionBool},

    {"autoraise_delay", CORE_NAME, "AutoRaiseInterval", "Windows", OptionInt},
    {"flip_time", "rotate", "ElectricBorderDelay", "Windows", OptionInt},
    {"number_of_desktops", CORE_NAME, "Number", "Desktops", OptionInt},

    {"unmaximize_window_key", CORE_NAME, NULL, "Windows", OptionSpecial},
    {"maximize_window_key", CORE_NAME, NULL, "Windows", OptionSpecial},
    {"maximize_window_horizontally_key", CORE_NAME, NULL, "Windows", OptionSpecial},
    {"maximize_window_vertically_key", CORE_NAME, NULL, "Windows", OptionSpecial},
    {"command11", "commands", NULL, "Windows", OptionSpecial},
    {"click_to_focus", CORE_NAME, NULL, "Windows", OptionSpecial},
    {"mode", "resize", NULL, "Windows", OptionSpecial},

    {"snap_type", "snap", NULL, "Windows", OptionSpecial},
    {"edges_categories", "snap", NULL, "Windows", OptionSpecial},
    {"resistance_distance", "snap", NULL, "Windows", OptionSpecial},
    {"attraction_distance", "snap", NULL, "Windows", OptionSpecial},

    {"next_key", "switcher", "Walk Through Windows", "Windows", OptionSpecial},
    {"prev_key", "switcher", "Walk Through Windows (Reverse)", "Windows", OptionSpecial},
    {"next_all_key", "switcher", "Walk Through Windows", "Windows", OptionSpecial},
    {"prev_all_key", "switcher", "Walk Through Windows (Reverse)", "Windows", OptionSpecial},
    {"next_no_popup_key", "switcher", "Walk Through Windows", "Windows", OptionSpecial},
    {"prev_no_popup_key", "switcher", "Walk Through Windows (Reverse)", "Windows", OptionSpecial},

    {"edge_flip_pointer", "rotate", "ElectricBorders",  "Windows", OptionSpecial},
    {"edge_flip_window", "rotate", "ElectricBorders",  "Windows", OptionSpecial},
    {"edgeflip_pointer", "wall", "ElectricBorders",  "Windows", OptionSpecial},
    {"edgeflip_move", "wall", "ElectricBorders",  "Windows", OptionSpecial},

    {"mode", "place", "Placement",  "Windows", OptionSpecial}

};

#define N_SOPTIONS (sizeof (specialOptions) / sizeof (struct _SpecialOption))


static void
createFile (QString name)
{
    if (!QFile::exists(name))
    {
	QFile file (name);
	file.open (QIODevice::WriteOnly | QIODevice::Append);
	file.close ();
    }
}

static void
reload (unsigned int,
	void     *closure)
{
    CCSContext *context = (CCSContext *) closure;

    ccsDisableFileWatch (cFiles->mainWatch);
    ccsDisableFileWatch (cFiles->kwinWatch);
    ccsDisableFileWatch (cFiles->shortcutWatch);
    cFiles->main->reparseConfiguration();
    cFiles->kwin->reparseConfiguration();
    cFiles->shortcuts->reparseConfiguration();
    ccsReadSettings (context);
    ccsEnableFileWatch (cFiles->mainWatch);
    ccsEnableFileWatch (cFiles->kwinWatch);
    ccsEnableFileWatch (cFiles->shortcutWatch);
}


static bool
isIntegratedOption (CCSSetting *setting)
{

    for (unsigned int i = 0; i < N_SOPTIONS; i++)
    {
	if (setting->name == specialOptions[i].settingName &&
	    QString (setting->parent->name) == specialOptions[i].pluginName)
		return true;
    }

    return false;
}

static void
KdeIntToCCS (CCSSetting   *setting,
	     int          num)
{
    int val = cFiles->kwin->group (specialOptions[num].groupName).
	       readEntry (specialOptions[num].kdeName,
			  setting->defaultValue.value.asInt);

    ccsSetInt (setting, val);
}

static void
KdeBoolToCCS (CCSSetting   *setting,
	      int          num)
{
    Bool val = (cFiles->kwin->group (specialOptions[num].groupName).
		readEntry (specialOptions[num].kdeName,
			   (setting->defaultValue.value.asBool)?
		true: false))? TRUE : FALSE;

    ccsSetBool (setting, val);
}

static void
KdeKeyToCCS (CCSSetting   *setting,
	     int          num)
{
    CCSSettingKeyValue keySet;
    keySet.keysym     = 0;
    keySet.keyModMask = 0;

    QStringList keyData = cFiles->shortcuts->
	group (specialOptions[num].groupName).
	readEntry (specialOptions[num].kdeName, QStringList());

    if (keyData.size () != 3)
	return;
    
    int key = QKeySequence(keyData[0].split (' ')[0])[0];

    int kdeKeymod = 0;

    if (key & Qt::ShiftModifier)
	kdeKeymod |= ShiftMask;

    if (key & Qt::ControlModifier)
	kdeKeymod |= ControlMask;

    if (key & Qt::AltModifier)
	kdeKeymod |= CompAltMask;

    if (key & Qt::MetaModifier)
	kdeKeymod |= CompSuperMask;

    keySet.keysym = XStringToKeysym(KShortcut (key & 0x1FFFFFF).toString ()
				    .toAscii ().constData ());
    keySet.keyModMask = kdeKeymod;
    ccsSetKey (setting, keySet);
}

static void
readIntegratedOption (CCSSetting   *setting,
		      KConfigGroup *mcg)
{
    int option = 0;
    KConfigGroup g;

    for (unsigned int i = 0; i < N_SOPTIONS; i++)
    {
	if (setting->name == specialOptions[i].settingName &&
	    QString (setting->parent->name) == specialOptions[i].pluginName)
	{
	    option = i;
	    break;
	}
    }

    switch (specialOptions[option].type)
    {

    case OptionInt:
	KdeIntToCCS (setting, option);
	break;

    case OptionBool:
	KdeBoolToCCS (setting, option);
	break;

    case OptionKey:
	KdeKeyToCCS (setting, option);
	break;

    case OptionSpecial:
	if (specialOptions[option].settingName == "command11")
	{
	    ccsSetString (setting, "xkill");
	}
	else if (specialOptions[option].settingName == "unmaximize_window_key"
		 || specialOptions[option].settingName == "maximize_window_key"
		 || specialOptions[option].settingName == "maximize_window_horizontally_key"
		 || specialOptions[option].settingName == "maximize_window_vertically_key")
	{
	    CCSSettingKeyValue keyVal;

	    if (!ccsGetKey (setting, &keyVal) )
		break;

	    keyVal.keysym = 0;

	    keyVal.keyModMask = 0;

	    ccsSetKey (setting, keyVal);
	}
	else if (specialOptions[option].settingName == "click_to_focus")
	{
	    Bool val = (cFiles->kwin->group ("Windows").
		        readEntry ("FocusPolicy") == "ClickToFocus") ?
		        TRUE : FALSE;
	    ccsSetBool (setting, val);
	}
	else if (specialOptions[option].settingName == "mode" &&
		 specialOptions[option].pluginName == "resize")
	{
	    QString mode = cFiles->kwin->group ("Windows").
			   readEntry ("ResizeMode");
	    int     imode = -1;
	    int     result = 0;

	    if (mcg->hasKey (specialOptions[option].settingName +
			     " (Integrated)"))
		imode = mcg->readEntry (specialOptions[option].settingName + 
				        " (Integrated)", int (0));

	    if (mode == "Opaque")
	    {
		result = 0;

		if (imode == 3)
		    result = 3;
	    }
	    else if (mode == "Transparent")
	    {
		result = 1;

		if (imode == 2)
		    result = 2;
	    }

	    ccsSetInt (setting, result);
	}
	else if (specialOptions[option].settingName == "snap_type")
	{
	    static int intList[2] = {0, 1};
	    CCSSettingValueList list = ccsGetValueListFromIntArray (intList, 2,
								    setting);
	    ccsSetList (setting, list);
	    ccsSettingValueListFree (list, TRUE);
	}
	else if (specialOptions[option].settingName == "resistance_distance" ||
		 specialOptions[option].settingName == "attraction_distance")
	{
	    int val1 = 
		cFiles->kwin->group ("Windows").
		    readEntry ("WindowSnapZone", int (0));
	    int val2 = 
		cFiles->kwin->group ("Windows").
		    readEntry ("BorderSnapZone", int (0));
	    int result = qMax (val1, val2);

	    if (result == 0)
		result = mcg->readEntry ("snap_distance (Integrated)", int (0));

	    if (result > 0)
	    	ccsSetInt (setting, result);
	}
	else if (specialOptions[option].settingName == "edges_categories")
	{
	    int val1 = 
		cFiles->kwin->group ("Windows").
		    readEntry ("WindowSnapZone", int (0));
	    int val2 = 
		cFiles->kwin->group ("Windows").
		    readEntry ("BorderSnapZone", int (0));
	    int intList[2] = {0, 0};
	    int num = 0;

	    if (val2 > 0)
		num++;
	    if (val1 > 0)
	    {
		intList[num] = 1;
		num++;
	    }

	    CCSSettingValueList list = ccsGetValueListFromIntArray (intList,
								    num,
								    setting);
	    ccsSetList (setting, list);
	    ccsSettingValueListFree (list, TRUE);
	}
	else if (specialOptions[option].settingName == "edge_flip_window" ||
		 specialOptions[option].settingName == "edgeflip_move")
	{
	    int val = 
		cFiles->kwin->group ("Windows").
		    readEntry ("ElectricBorders", int (0));

	    if (val > 0)
		ccsSetBool (setting, TRUE);
	    else
		ccsSetBool (setting, FALSE);
	}
	else if (specialOptions[option].settingName == "edge_flip_pointer" ||
		 specialOptions[option].settingName == "edgeflip_pointer")
	{
	    int val = 
		cFiles->kwin->group ("Windows").
		    readEntry ("ElectricBorders", int (0));

	    if (val > 1)
		ccsSetBool (setting, TRUE);
	    else
		ccsSetBool (setting, FALSE);
	}
	else if (specialOptions[option].settingName == "mode" &&
		 specialOptions[option].pluginName == "place")
	{
	    QString mode = cFiles->kwin->group ("Windows").
			   readEntry ("Placement");
	    int     result = 0;

	    if (mode == "Smart")
		result = 2;
	    else if (mode == "Maximizing")
		result = 3;
	    else if (mode == "Cascade")
		result = 0;
	    else if (mode == "Random")
		result = 4;
	    else if (mode == "Centered")
		result = 1;

	    ccsSetInt (setting, result);
	}
	break;

    default:
	break;
    }
}

static Bool
getSettingIsIntegrated (CCSSetting *setting)
{
    return ccsGetIntegrationEnabled (setting->parent->context)
	   && isIntegratedOption (setting);
}


static Bool
getSettingIsReadOnly (CCSSetting *setting)
{
    if (!ccsGetIntegrationEnabled (setting->parent->context)
	|| !isIntegratedOption (setting) )
	return FALSE;

    int option = 0;

    for (unsigned int i = 0; i < N_SOPTIONS; i++)
    {
	if (setting->name == specialOptions[i].settingName &&
	    QString (setting->parent->name) == specialOptions[i].pluginName)
	{
	    option = i;
	    break;
	}
    }

    switch (specialOptions[option].type)
    {
    case OptionSpecial:
	if (specialOptions[option].settingName == "command11")
	{
	    return TRUE;
	}
	else if (specialOptions[option].settingName == "map_on_shutdown")
	{
	    return TRUE;
	}
	else if (specialOptions[option].settingName == "unmaximize_window_key"
		 || specialOptions[option].settingName == "maximize_window_key"
		 || specialOptions[option].settingName == "maximize_window_horizontally_key"
		 || specialOptions[option].settingName == "maximize_window_vertically_key")
	{
	    return TRUE;
	}
	else if (specialOptions[option].settingName == "snap_type" ||
		 specialOptions[option].settingName == "attraction_distance")
	{
	    return TRUE;
	}
	break;

    default:
	break;
    }

    return FALSE;
}

static CCSStringList
getExistingProfiles (CCSContext *)
{
    QDir dir (KGlobal::dirs()->saveLocation ("config", QString::null, false),
	      				     "compizrc.*");

    QStringList files = dir.entryList();
    CCSStringList ret = NULL;

    QStringList::iterator it;

    for (it = files.begin(); it != files.end(); it++)
    {
	QString str = (*it);

	if (str.length() > 9)
	{
	    QString profile = str.right (str.length() - 9);

	    if (!profile.isEmpty() )
		ret = ccsStringListAppend (ret, 
		    strdup (profile.toAscii().constData()));
	}
    }

    return ret;
}

static void
readSetting (CCSContext *c,
	     CCSSetting *setting)
{
    QString key (setting->name);
    QString group (setting->parent->name);

    if (setting->isScreen)
    {
	group += "_screen";
	group += QString::number (setting->screenNum);
    }
    else
	group += "_display";

    KConfigGroup cfg = cFiles->main->group (group);

    if (ccsGetIntegrationEnabled (c) && isIntegratedOption (setting) )
    {
	readIntegratedOption (setting, &cfg);
	return;
    }

    if (!cfg.hasKey (key) )
    {
	ccsResetToDefault (setting);
	return;
    }

    switch (setting->type)
    {

    case TypeString:
	ccsSetString (setting, cfg.readEntry (key, "").toAscii ().constData ());
	break;

    case TypeMatch:
	ccsSetMatch (setting, cfg.readEntry (key, "").toAscii ().constData ());
	break;

    case TypeFloat:
	ccsSetFloat (setting, cfg.readEntry (key, double(0.0)));
	break;

    case TypeInt:
	ccsSetInt (setting, cfg.readEntry (key, int(0)));
	break;

    case TypeBool:
	{
	    Bool val = (cfg.readEntry (key, false)) ? TRUE : FALSE;
	    ccsSetBool (setting, val);
	}
	break;

    case TypeColor:
	{
	    CCSSettingColorValue color;
	    QString value = cfg.readEntry (key, "");

	    if (ccsStringToColor (value.toAscii ().constData (), &color))
		ccsSetColor (setting, color);
	}
	break;

    case TypeList:
	{
	    switch (setting->info.forList.listType)
	    {

	    case TypeBool:
		{
		    QList<bool> list = cfg.readEntry (key, QList<bool> ());
		    Bool array[list.count ()];
		    int i = 0;

		    foreach (Bool val, list)
		    {
			array[i] = (val) ? TRUE : FALSE;
			i++;
		    }

		    CCSSettingValueList l =
			ccsGetValueListFromBoolArray (array, i, setting);
		    ccsSetList (setting, l);
		    ccsSettingValueListFree (l, TRUE);
		}
		break;

	    case TypeInt:
		{
		    QList<int> list = cfg.readEntry (key, QList<int> ());
		    int array[list.count ()];
		    int i = 0;

		    foreach (int val, list)
		    {
			array[i] = val;
			i++;
		    }

		    CCSSettingValueList l =
			ccsGetValueListFromIntArray (array, i, setting);
		    ccsSetList (setting, l);
		    ccsSettingValueListFree (l, TRUE);
		}
		break;

	    case TypeString:
		{
		    QStringList list = cfg.readEntry (key, QStringList ());

		    if (!list.count ())
			break;

		    char *array[list.count ()];
		    int i = 0;

		    foreach (QString val, list)
		    {
			array[i] = strdup (val.toAscii ().constData ());
			i++;
		    }

		    CCSSettingValueList l =
			ccsGetValueListFromStringArray (array, i, setting);
		    ccsSetList (setting, l);
		    ccsSettingValueListFree (l, TRUE);

		    for (int j = 0; j < i; j++)
			free (array[j]);
		}
		break;

	    case TypeMatch:
		{
		    QStringList list = cfg.readEntry (key, QStringList ());

		    if (!list.count ())
			break;

		    char *array[list.count ()];
		    int i = 0;

		    foreach (QString val, list)
		    {
			array[i] = strdup (val.toAscii ().constData ());
			i++;
		    }

		    CCSSettingValueList l =
			ccsGetValueListFromStringArray (array, i, setting);
		    ccsSetList (setting, l);
		    ccsSettingValueListFree (l, TRUE);

		    for (int j = 0; j < i; j++)
			free (array[j]);
		}
		break;

	    case TypeFloat:
		{
		    QList<float> list = cfg.readEntry (key, QList<float> ());
		    float array[list.count ()];
		    int   i = 0;

		    foreach (float val, list)
		    {
			array[i] = val;
			i++;
		    }

		    CCSSettingValueList l =
			ccsGetValueListFromFloatArray (array, i, setting);
		    ccsSetList (setting, l);
		    ccsSettingValueListFree (l, TRUE);
		}
		break;

	    case TypeColor:
		{
		    QStringList list = cfg.readEntry (key, QStringList ());
		    CCSSettingColorValue array[list.count ()];
		    int i = 0;

		    foreach (QString val, list)
		    {
			if (!ccsStringToColor (val.toAscii ().constData (),
					       &array[i]))
			{
			    memset (&array[i], 0,
				    sizeof (CCSSettingColorValue));
			    array[i].color.alpha = 0xffff;
			}
			i++;
		    }

		    CCSSettingValueList l =
			ccsGetValueListFromColorArray (array, i, setting);
		    ccsSetList (setting, l);
		    ccsSettingValueListFree (l, TRUE);
		}
		break;

	    case TypeKey:
		{
		    QStringList list = cfg.readEntry (key, QStringList ());

		    CCSSettingValue     *sVal = NULL;
		    CCSSettingValueList l = NULL;

		    foreach (QString val, list)
		    {
			sVal = (CCSSettingValue*) malloc (sizeof (CCSSettingValue));
			if (!sVal)
			    break;
			if (ccsStringToKeyBinding (val.toAscii ().constData (),
			    			   &sVal->value.asKey))
			    l = ccsSettingValueListAppend (l, sVal);
			else
			    free (sVal);
		    }

		    ccsSetList (setting, l);
		    ccsSettingValueListFree (l, TRUE);
		}
		break;
	    case TypeButton:
		{
		    QStringList list = cfg.readEntry (key, QStringList ());

		    CCSSettingValue     *sVal = NULL;
		    CCSSettingValueList l = NULL;

		    foreach (QString val, list)
		    {
			sVal = (CCSSettingValue*) malloc (sizeof (CCSSettingValue));
			if (!sVal)
			    break;
			if (ccsStringToButtonBinding (
				val.toAscii ().constData (),
				&sVal->value.asButton))
			    l = ccsSettingValueListAppend (l, sVal);
			else
			    free (sVal);
		    }

		    ccsSetList (setting, l);
		    ccsSettingValueListFree (l, TRUE);
		}
		break;
	    case TypeEdge:
		{
		    QStringList list = cfg.readEntry (key, QStringList ());

		    CCSSettingValue     *sVal = NULL;
		    CCSSettingValueList l = NULL;

		    foreach (QString val, list)
		    {
			sVal = (CCSSettingValue*) malloc (sizeof (CCSSettingValue));
			if (!sVal)
			    break;
			sVal->value.asEdge =
			    ccsStringToEdges (val.toAscii ().constData ());
			l = ccsSettingValueListAppend (l, sVal);
		    }

		    ccsSetList (setting, l);
		    ccsSettingValueListFree (l, TRUE);
		}
		break;
	    case TypeBell:
		{
		    QList<bool> list = cfg.readEntry (key, QList<bool> ());

		    CCSSettingValue     *sVal = NULL;
		    CCSSettingValueList l = NULL;

		    foreach (bool val, list)
		    {
			sVal = (CCSSettingValue*) malloc (sizeof (CCSSettingValue));
			if (!sVal)
			    break;
			sVal->value.asBell = (val) ? TRUE : FALSE;
			l = ccsSettingValueListAppend (l, sVal);
		    }

		    ccsSetList (setting, l);
		    ccsSettingValueListFree (l, TRUE);
		}
		break;

	    default:
		break;
	    }
	}
	break;

    case TypeKey:
	{
	    QString str = cfg.readEntry (key, QString ());

	    CCSSettingKeyValue value;

	    ccsStringToKeyBinding (str.toAscii ().constData (), &value);

	    ccsSetKey (setting, value);
	}
	break;
    case TypeButton:
	{
	    QString str = cfg.readEntry (key, QString ());

	    CCSSettingButtonValue value;

	    ccsStringToButtonBinding (str.toAscii ().constData (), &value);

	    ccsSetButton (setting, value);
	}
	break;
    case TypeEdge:
	{
	    QString str = cfg.readEntry (key, QString ());

	    unsigned int value;

	    value = ccsStringToEdges (str.toAscii ().constData ());

	    ccsSetEdge (setting, value);
	}
	break;
    case TypeBell:
	{
	    Bool val = (cfg.readEntry (key, false)) ? TRUE : FALSE;
	    ccsSetBell (setting, val);
	}
	break;

    default:
	kDebug () << "Not supported setting type : " << setting->type << endl;
	break;
    }
}

static void
CCSIntToKde (CCSSetting *setting,
	     int        num)
{
    KConfigGroup g = cFiles->kwin->group (specialOptions[num].groupName);

    int val;

    if (!ccsGetInt (setting, &val) )
	return;

    if (g.readEntry (specialOptions[num].kdeName, ~val) != val)
    {
	cFiles->modified = true;
	g.writeEntry (specialOptions[num].kdeName, val);
    }
}

static void
CCSBoolToKde (CCSSetting *setting,
	      int        num)
{
    KConfigGroup g = cFiles->kwin->group (specialOptions[num].groupName);

    Bool val;

    if (!ccsGetBool (setting, &val) )
	return;

    if (g.readEntry (specialOptions[num].kdeName, bool (~val)) != bool (val) )
    {
	cFiles->modified = true;
	g.writeEntry (specialOptions[num].kdeName, bool (val) );
    }
}

static void
CCSKeyToKde (CCSSetting *setting,
	     int        num)
{

    CCSSettingKeyValue keyVal;

    if (!ccsGetKey (setting, &keyVal) )
        return;

    int key = QKeySequence (XKeysymToString (keyVal.keysym));

    if (keyVal.keyModMask & ShiftMask)
	key |= Qt::ShiftModifier;

    if (keyVal.keyModMask & ControlMask)
	key |= Qt::ControlModifier;

    if (keyVal.keyModMask & CompAltMask)
	key |= Qt::AltModifier;

    if (keyVal.keyModMask & CompSuperMask)
	key |= Qt::MetaModifier;


    QStringList keyData = cFiles->shortcuts->
	group (specialOptions[num].groupName).
	readEntry (specialOptions[num].kdeName, QStringList());

    if (keyData.size () != 3)
	return;

    QStringList kl = keyData[0].split (' ');

    kl[0] = (key)? QKeySequence(key).toString () : "none";

    keyData[0] = kl.join (" ");

    cFiles->shortcuts->group (specialOptions[num].groupName).
	writeEntry (specialOptions[num].kdeName, keyData);
    cFiles->modified = true;
}


static void
writeIntegratedOption (CCSSetting *setting)
{
    int option = 0;

    for (unsigned int i = 0; i < N_SOPTIONS; i++)
    {
	if (setting->name == specialOptions[i].settingName &&
	    QString (setting->parent->name) == specialOptions[i].pluginName)
	{
	    option = i;
	    break;
	}
    }

    QString group (setting->parent->name);

    if (setting->isScreen)
    {
	group += "_screen";
	group += QString::number (setting->screenNum);
    }
    else
	group += "_display";

    KConfigGroup cfg = cFiles->main->group (group);

    switch (specialOptions[option].type)
    {

    case OptionInt:
	CCSIntToKde (setting, option);
	break;

    case OptionBool:
	CCSBoolToKde (setting, option);
	break;
    case OptionKey:
	CCSKeyToKde (setting, option);
	break;

    case OptionSpecial:
	if (specialOptions[option].settingName == "command11"
	    || specialOptions[option].settingName == "unmaximize_window_key"
	    || specialOptions[option].settingName == "maximize_window_key"
	    || specialOptions[option].settingName == "maximize_window_horizontally_key"
	    || specialOptions[option].settingName == "maximize_window_vertically_key")
	    break;

	if (specialOptions[option].settingName == "click_to_focus")
	{
	    QString mode = cFiles->kwin->group ("Windows").
			   readEntry ("FocusPolicy");
	    QString val = "ClickToFocus";
	    Bool bVal;

	    if (!ccsGetBool (setting, &bVal) )
		break;

	    if (!bVal)
	    {
		val = "FocusFollowsMouse";
	    }

	    if (mode != val)
	    {
		cFiles->modified = true;
		cFiles->kwin->group ("Windows").writeEntry ("FocusPolicy", val);
	    }
	}
	if (specialOptions[option].settingName == "mode" &&
	    specialOptions[option].pluginName == "resize")
	{
	    QString mode = cFiles->kwin->group ("Windows").
			   readEntry("ResizeMode");
	    QString val = "Opaque";
	    int     iVal;
	    if (ccsGetInt(setting, &iVal) && (iVal == 1 || iVal == 2))
	    {
		val = "Transparent";
	    }
	    if (mode != val)
	    {
		cFiles->modified = true;
		cFiles->kwin->group ("Windows").writeEntry("ResizeMode",val);
	    }
	    cfg.writeEntry(specialOptions[option].settingName + " (Integrated)",iVal);
	}

	if (specialOptions[option].settingName == "resistance_distance" ||
	    specialOptions[option].settingName == "edges_categories")
	{
	    int *values, numValues;
	    CCSSettingValueList sList;

	    bool edge = false;
	    bool window = false;

	    int iVal = 0;

	    CCSSetting *edges = ccsFindSetting(setting->parent,
					       "edges_categories",
					       setting->isScreen,
					       setting->screenNum);

	    CCSSetting *dist = ccsFindSetting(setting->parent,
					      "resistance_distance",
					      setting->isScreen,
					      setting->screenNum);

	    if (!edges || !dist || !ccsGetList (edges, &sList) ||
		!ccsGetInt(dist, &iVal))
		break;

	    values = ccsGetIntArrayFromValueList (sList, &numValues);

	    for (int i = 0; i < numValues; i++)
	    {
		if (values[i] == 0)
		    edge = true;
		if (values[i] == 1)
		    window = true;
	    }

	    if (values)
		free (values);

	    if (edge)
		cFiles->kwin->group ("Windows").
		    writeEntry ("BorderSnapZone", iVal);
	    else
		cFiles->kwin->group ("Windows").
		    writeEntry ("BorderSnapZone", 0);

	    if (window)
		cFiles->kwin->group ("Windows").
		    writeEntry ("WindowSnapZone", iVal);
	    else
		cFiles->kwin->group ("Windows").
		    writeEntry ("WindowSnapZone", 0);

	    if (window | edge)
		cFiles->modified = true;
	    cfg.writeEntry ("snap_distance (Integrated)",iVal);
	}
	else if (specialOptions[option].settingName == "next_key" ||
		 specialOptions[option].settingName == "prev_key")
	{
	    CCSSettingKeyValue keyVal;

	    if (!ccsGetKey (setting, &keyVal))
		break;

	    if (keyVal.keysym == 0 && keyVal.keyModMask == 0)
		break;

	    CCSKeyToKde (setting, option);

	    cFiles->kwin->group ("TabBox").writeEntry ("TraverseAll", false);
	    cFiles->kwin->group ("Windows").writeEntry ("AltTabStyle", "KDE");

	    cFiles->modified = true;
	}
	else if (specialOptions[option].settingName == "next_all_key" ||
		 specialOptions[option].settingName == "prev_all_key")
	{
	    CCSSettingKeyValue keyVal;

	    if (!ccsGetKey (setting, &keyVal))
		break;

	    if (keyVal.keysym == 0 && keyVal.keyModMask == 0)
		break;

	    CCSKeyToKde (setting, option);

	    cFiles->kwin->group ("TabBox").writeEntry ("TraverseAll", true);
	    cFiles->kwin->group ("Windows").writeEntry ("AltTabStyle", "KDE");

	    cFiles->modified = true;
	}
	else if (specialOptions[option].settingName == "next_no_popup_key" ||
		 specialOptions[option].settingName == "prev_no_popup_key")
	{
	    CCSSettingKeyValue keyVal;

	    if (!ccsGetKey (setting, &keyVal))
		break;

	    if (keyVal.keysym == 0 && keyVal.keyModMask == 0)
		break;

	    CCSKeyToKde (setting, option);

	    cFiles->kwin->group ("Windows").writeEntry ("AltTabStyle", "CDE");

	    cFiles->modified = true;
	}
	else if (specialOptions[option].settingName == "edge_flip_window" ||
		 specialOptions[option].settingName == "edgeflip_move")
	{
	    int  oVal = cFiles->kwin->group ("Windows").
			readEntry ("ElectricBorders", 0);
	    Bool val;

	    if (!ccsGetBool (setting, &val))
		break;

	    if (val)
		cFiles->kwin->group ("Windows").
		    writeEntry ("ElectricBorders", qMax (1, oVal));
	    else
		cFiles->kwin->group ("Windows").
		    writeEntry ("ElectricBorders", 0);
	    cFiles->modified = true;
	}
	else if (specialOptions[option].settingName == "edge_flip_pointer" ||
		 specialOptions[option].settingName == "edgeflip_pointer")
	{
	    int  oVal = 0;
	    Bool val, val2;

	    if (!ccsGetBool (setting, &val))
		break;

	    CCSSetting *valSet = ccsFindSetting(setting->parent,
						 "edge_flip_window",
						 setting->isScreen,
						 setting->screenNum);

	    if (!valSet)
	     	valSet = ccsFindSetting(setting->parent, "edgeflip_move",
					setting->isScreen, setting->screenNum);

	    if (valSet && ccsGetBool (valSet, &val2))
	    {
		if (val2)
		    oVal = 1;
	    }
	    else
		oVal = 0;
		

	    if (val)
		cFiles->kwin->group ("Windows").
		    writeEntry ("ElectricBorders", 2);
	    else
		cFiles->kwin->group ("Windows").
		    writeEntry ("ElectricBorders", oVal);
	    cFiles->modified = true;
	}
	else if (specialOptions[option].settingName == "mode" &&
		 specialOptions[option].pluginName == "place")
	{
	    int val;
	    if (!ccsGetInt (setting, &val))
		break;

	    switch (val)
	    {
	    case 0:
		cFiles->kwin->group ("Windows").
		    writeEntry ("Placement", "Cascade");
		break;
	    case 1:
		cFiles->kwin->group ("Windows").
		    writeEntry ("Placement", "Centered");
		break;
	    case 2:
		cFiles->kwin->group ("Windows").
		    writeEntry ("Placement", "Smart");
		break;
	    case 3:
		cFiles->kwin->group ("Windows").
		    writeEntry ("Placement", "Maximizing");
		break;
	    case 4:
		cFiles->kwin->group ("Windows").
		    writeEntry ("Placement", "Random");
		break;
	    default:
		break;
	    }

	    cFiles->modified = true;
	}
	break;
    default:
	break;
    }
}


static void
writeSetting (CCSContext *c,
	      CCSSetting *setting)
{
    QString key (setting->name);
    QString group (setting->parent->name);

    if (setting->isScreen)
    {
	group += "_screen";
	group += QString::number (setting->screenNum);
    }
    else
	group += "_display";

    KConfigGroup cfg = cFiles->main->group (group);

    if (ccsGetIntegrationEnabled (c) && isIntegratedOption (setting) )
    {
	writeIntegratedOption (setting);
	return;
    }

    switch (setting->type)
    {

    case TypeString:
	{
	    char * val;

	    if (ccsGetString (setting, &val) )
		cfg.writeEntry (key, QString (val));
	}
	break;

    case TypeMatch:
	{
	    char * val;

	    if (ccsGetMatch (setting, &val) )
		cfg.writeEntry (key, QString (val));
	}
	break;

    case TypeFloat:
	{
	    float val;

	    if (ccsGetFloat (setting, &val) )
		cfg.writeEntry (key, double(val));
	}
	break;

    case TypeInt:
	{
	    int val;

	    if (ccsGetInt (setting, &val) )
		cfg.writeEntry (key, val);
	}
	break;

    case TypeBool:
	{
	    Bool val;

	    if (ccsGetBool (setting, &val) )
		cfg.writeEntry (key, bool (val) );
	}
	break;

    case TypeColor:
	{
	    CCSSettingColorValue color;
	    char *value;

	    if (!ccsGetColor (setting, &color))
		break;

	    value = ccsColorToString (&color);
	    if (value)
		cfg.writeEntry (key, QString (value));
	    free (value);
	}
	break;

    case TypeList:
	{
	    switch (setting->info.forList.listType)
	    {

	    case TypeBool:
		{
		    QList<int> list;
		    CCSSettingValueList l;

		    if (!ccsGetList (setting, &l) )
			break;

		    while (l)
		    {
			list.append (l->data->value.asBool);
			l = l->next;
		    }

		    cfg.writeEntry (key, list);
		}
		break;
		
	    case TypeInt:
		{
		    QList<int> list;
		    CCSSettingValueList l;

		    if (!ccsGetList (setting, &l) )
			break;

		    while (l)
		    {
			list.append (l->data->value.asInt);
			l = l->next;
		    }

		    cfg.writeEntry (key, list);
		}
		break;

	    case TypeString:
		{
		    QStringList list;
		    CCSSettingValueList l;

		    if (!ccsGetList (setting, &l) )
			break;

		    while (l)
		    {
			list.append (l->data->value.asString);
			l = l->next;
		    }

		    cfg.writeEntry (key, list);
		}
		break;

	    case TypeMatch:
		{
		    QStringList list;
		    CCSSettingValueList l;

		    if (!ccsGetList (setting, &l) )
			break;

		    while (l)
		    {
			list.append (l->data->value.asMatch);
			l = l->next;
		    }

		    cfg.writeEntry (key, list);
		}
		break;

	    case TypeFloat:
		{
		    QStringList list;
		    CCSSettingValueList l;

		    if (!ccsGetList (setting, &l) )
			break;

		    while (l)
		    {
			list.append (QString::number (l->data->value.asFloat) );
			l = l->next;
		    }

		    cfg.writeEntry (key, list);
		}
		break;

	    case TypeColor:
		{
		    QStringList list;
		    CCSSettingValueList l;

		    if (!ccsGetList (setting, &l) )
			break;

		    while (l)
		    {
			char *value;
			
			value = ccsColorToString (&l->data->value.asColor);
			if (value)
			    list.append (QString (value));
			free (value);

			l = l->next;
		    }

		    cfg.writeEntry (key, list);
		}
		break;
	    case TypeKey:
		{
		    QStringList list;
		    CCSSettingValueList l;

		    if (!ccsGetList (setting, &l) )
			break;

		    while (l)
		    {
			QString str;
			char *val;
			val = ccsKeyBindingToString (&l->data->value.asKey);

			str = val;
			free (val);
			
			list.append (str);
			l = l->next;
		    }

		    cfg.writeEntry (key, list);
		}
		break;
	    case TypeButton:
		{
		    QStringList list;
		    CCSSettingValueList l;

		    if (!ccsGetList (setting, &l) )
			break;

		    while (l)
		    {
			QString str;
			char *val;
			val = ccsButtonBindingToString (
			          &l->data->value.asButton);

			str = val;
			free (val);
			
			list.append (str);
			l = l->next;
		    }

		    cfg.writeEntry (key, list);
		}
		break;
	    case TypeEdge:
		{
		    QStringList list;
		    CCSSettingValueList l;

		    if (!ccsGetList (setting, &l) )
			break;

		    while (l)
		    {
			QString str;
			char *val;
			val = ccsEdgesToString (l->data->value.asEdge);

			str = val;
			free (val);
			
			list.append (str);
			l = l->next;
		    }

		    cfg.writeEntry (key, list);
		}
		break;
	    case TypeBell:
		{
		    QList<int> list;
		    CCSSettingValueList l;

		    if (!ccsGetList (setting, &l) )
			break;

		    while (l)
		    {
			list.append (l->data->value.asBell);
			l = l->next;
		    }

		    cfg.writeEntry (key, list);
		}
		break;
	    default:
		break;
	    }
	}
	break;
	
    case TypeKey:
	{
	    CCSSettingKeyValue keyVal;

	    if (!ccsGetKey (setting, &keyVal))
		break;

	    char *val = ccsKeyBindingToString (&keyVal);

	    cfg.writeEntry (key, QString (val));

	    free (val);
	}
	break;
    case TypeButton:
	{
	    CCSSettingButtonValue buttonVal;

	    if (!ccsGetButton (setting, &buttonVal))
		break;

	    char *val = ccsButtonBindingToString (&buttonVal);

	    cfg.writeEntry (key, QString (val));

	    free (val);
	}
	break;
    case TypeEdge:
	{
	    unsigned int edges;

	    if (!ccsGetEdge (setting, &edges) )
		break;

	    char *val = ccsEdgesToString (edges);

	    cfg.writeEntry (key, QString (val));

	    free (val);
	}
	break;
    case TypeBell:
	{
	    Bool bell;

	    if (!ccsGetBell (setting, &bell))
		break;

	    cfg.writeEntry (key, (bell)? true : false);
	}
	break;

    default:
	kDebug () << "Not supported setting type : " << setting->type << endl;
	break;
    }
}

static Bool
readInit (CCSContext *c)
{
    if (cFiles->profile != ccsGetProfile (c))
    {
	QString configName ("compizrc");

	if (ccsGetProfile (c) && strlen (ccsGetProfile (c)))
	{
	    configName += ".";
	    configName += ccsGetProfile (c);
	    cFiles->profile = ccsGetProfile (c);
	}

	delete cFiles->main;

	QString wFile = KGlobal::dirs ()->saveLocation ("config",
			QString::null, false) + configName;
	createFile (wFile);
	
	cFiles->main = new KConfig (configName);
	ccsRemoveFileWatch (cFiles->mainWatch);
	cFiles->mainWatch = ccsAddFileWatch (wFile.toAscii ().constData (),
					     TRUE, reload, (void *) c);
    }

    return TRUE;
}

static void
readDone (CCSContext *)
{}

static Bool
writeInit (CCSContext *c)
{
    if (cFiles->profile != ccsGetProfile (c))
    {
	QString configName ("compizrc");

	if (ccsGetProfile (c) && strlen (ccsGetProfile (c)))
	{
	    configName += ".";
	    configName += ccsGetProfile (c);
	    cFiles->profile = ccsGetProfile (c);
	}

	delete cFiles->main;

	QString wFile = KGlobal::dirs ()->saveLocation ("config",
			QString::null, false) + configName;
	
	createFile (wFile);
	
	cFiles->main = new KConfig (configName);
	ccsRemoveFileWatch (cFiles->mainWatch);
	cFiles->mainWatch = ccsAddFileWatch (wFile.toAscii ().constData (),
					     TRUE, reload, (void *) c);
    }

    ccsDisableFileWatch (cFiles->mainWatch);
    ccsDisableFileWatch (cFiles->kwinWatch);
    ccsDisableFileWatch (cFiles->shortcutWatch);

    return TRUE;
}

static void
writeDone (CCSContext *)
{
    cFiles->main->sync();

    if (cFiles->modified)
    {
	cFiles->kwin->sync();
	cFiles->shortcuts->sync();

	org::kde::KWin kwin ("org.kde.kwin", "/KWin", 
			     QDBusConnection::sessionBus());
        kwin.reconfigure();

	cFiles->modified = false;
    }

    ccsEnableFileWatch (cFiles->mainWatch);
    ccsEnableFileWatch (cFiles->kwinWatch);
    ccsEnableFileWatch (cFiles->shortcutWatch);
}

static Bool
init (CCSContext *c)
{
    QString configName ("compizrc");

    cFiles = new ConfigFiles();
    
    if (ccsGetProfile (c) && strlen (ccsGetProfile (c)))
    {
	configName += ".";
	configName += ccsGetProfile (c);
	cFiles->profile = ccsGetProfile (c);
    }

    QString wFile = KGlobal::dirs ()->saveLocation ("config",
		    QString::null, false) + configName;

    createFile (wFile);

    cFiles->main      = new KConfig (configName);
    cFiles->kwin      = new KConfig ("kwinrc");
    cFiles->shortcuts = new KConfig ("kglobalshortcutsrc");


    cFiles->mainWatch = ccsAddFileWatch (wFile.toAscii ().constData (), TRUE,
					 reload, (void *) c);

    wFile = KGlobal::dirs ()->saveLocation ("config", QString::null, false) + 
		"kwinrc";
    cFiles->kwinWatch = ccsAddFileWatch (wFile.toAscii ().constData (),
					 TRUE, reload, (void *) c);

    wFile = KGlobal::dirs ()->saveLocation ("config", QString::null, false) + 
		"kglobalshortcutsrc";
    cFiles->shortcutWatch = ccsAddFileWatch (wFile.toAscii ().constData (),
					     TRUE, reload, (void *) c);

    return TRUE;
}

static Bool
fini (CCSContext *)
{
    if (cFiles)
    {
	ccsRemoveFileWatch (cFiles->mainWatch);
	ccsRemoveFileWatch (cFiles->kwinWatch);
	ccsRemoveFileWatch (cFiles->shortcutWatch);
	
	if (cFiles->main)
	    delete cFiles->main;

	if (cFiles->kwin)
	    delete cFiles->kwin;

	if (cFiles->shortcuts)
	    delete cFiles->shortcuts;


	delete cFiles;
    }

    cFiles = NULL;

    return TRUE;
}

static Bool
deleteProfile (CCSContext *,
	       char       *profile)
{
    QString file (KGlobal::dirs()->saveLocation ("config",
		  QString::null, false) );
    file += "compizrc";

    if (profile && strlen (profile) )
    {
	file += ".";
	file += profile;
    }

    if (QFile::exists (file) )
	return QFile::remove (file);

    return FALSE;
}

static CCSBackendVTable kconfigVTable =
{
    (char *) "kconfig4",
    (char *) "KDE4 Configuration Backend",
    (char *) "KDE4 Configuration Backend for libcompizconfig",
    true,
    true,
    0,
    init,
    fini,
    readInit,
    readSetting,
    readDone,
    writeInit,
    writeSetting,
    writeDone,
    getSettingIsIntegrated,
    getSettingIsReadOnly,
    getExistingProfiles,
    deleteProfile
};

extern "C"
{

    KDE_EXPORT CCSBackendVTable *
    getBackendInfo (void)
    {
	KComponentData componentData ("ccs-backend-kconfig4");
	return &kconfigVTable;
    }

}
