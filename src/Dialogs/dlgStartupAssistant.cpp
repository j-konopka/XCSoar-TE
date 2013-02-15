/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#include "Dialogs/Dialogs.h"
#include "Form/Button.hpp"
#include "Form/Form.hpp"
#include "Dialogs/WidgetDialog.hpp"
#include "Form/Frame.hpp"
#include "ComputerSettings.hpp"
#include "Look/InfoBoxLook.hpp"
#include "Look/Look.hpp"
#include "Look/DialogLook.hpp"
#include "UIGlobals.hpp"
#include "Form/ButtonPanel.hpp"
#include "Language/Language.hpp"
#include "Screen/SingleWindow.hpp"
#include "Screen/Layout.hpp"
#include "Profile/Profile.hpp"
#include "DisplaySettings.hpp"
#include "Util/Macros.hpp"
#include "Interface.hpp"
#include "UISettings.hpp"
#include "Form/CheckBox.hpp"
#include "Version.hpp"
#include "Form/Widget.hpp"
#include "Screen/Fonts.hpp"
#include "Util/StaticString.hpp"
#include "Task/TaskNationalities.hpp"
#include "Task/TaskBehaviour.hpp"
#include "Dialogs/Message.hpp"


const TCHAR* tips [] = {
    N_("Click the Nav Bar at the top to show the navigation menu."),
    N_("Slide the Nav Bar to the left or right to advance the task turnpoint."),
    N_("Click the 'M' button to show the Main menu.  Try clicking it again."),
    N_("To cancel a menu, click anywhere on the screen in the background to hide it."),
    N_("While flying, you should be able to do anything from either the Nav Bar or the primary 'M' menu."),
    N_("After you've entered a task cylinder, the Nav Bar shows a checkmark by the turnpoint name."),
    N_("If you are flying a MAT task, just create the start and finish.  Task points will be added as you fly over them."),
    N_("Add your glider's polar with the 'Plane' menu option."),
    N_("Enter the pilot's name for the .IGC file with the 'Setup' menu and select 'Setup Logger.'"),
    N_("Set your safety height margin for arrival with the 'Setup' menu and select 'Glide computer, Safety factors.'"),
    N_("Select your GPS device with the 'Device' menu."),
    N_("Create tasks in See You desktop and save them as a .CUP file.  Copy the .CUP file to XCSoarData.  Load using Top Hat's task manager."),
    N_("Click on an item in the map to show its details."),
    N_("Click on a waypoint on the map to Goto it."),
    N_("Click on the a waypoint on the map to Goto it."),
    N_("Click on an InfoBox to see what it does, or change its function."),
    N_("Click on the Wind InfoBox to change your wind settings."),
    N_("Click on the MacCready InfoBox to change your MacCready setting."),
    N_("You must have the MacCready InfoBox visible to change the MacCready setting.  Unless you have a logger that changes Top Hat's MacCready setting."),
    N_("If you increase your MacCready setting, the map will increase the altitude you need to reach any waypoint.  It assumes you fly at the appropriate MacCready speed."),
    N_("Set your Contest Nationality using the 'Setup' menu, then click Contest.  This will make it easier for you to create tasks by showing only options for your country."),
    N_("Set your Contest Nationality using the 'Setup' menu, then click Contest.  This will make it easier for you to create tasks by showing only options for your country."),
    N_("Altitudes displayed on the map are the altitudes required to reach the waypoint at your safety height flying at the current MacCready setting / MacCready speed."),
    N_("If you're flying a task, the final glide bar on the left indicates your arrival height above the finish height plus your safety height."),
    N_("The big arrow on the left indicates the amount of altitude above or below final glide to your destination, or to complete a task if you are flying a task."),
    N_("The big arrow on the left indicates the amount of altitude above or below final glide to your destination, or to complete a task if you are flying a task."),
    N_("To enable / disable the 'glide amoeba' click on the menu 'System options' then click on 'Glide computer, Reach.'"),
    N_("Too many waypoint labels?  Not enough?  Click the 'Labels...' button on the menu to toggle more, fewer, less options.'"),
};

enum ControlIndex {
  TipText,
  DeclineCheckbox,
  NextTip,
  PrevTip,
  CloseDialog,
};

enum Actions {
  NextTipClick = 100,
  PrevTipClick,
  DeclineCheckboxClick,
  CloseDialogClick,
};

gcc_const
static WindowStyle
GetDialogStyle()
{
  WindowStyle style;
  style.Hide();
  style.ControlParent();
  return style;
}

class StartupAssistant : public NullWidget, public WndForm
{
public:
  PixelRect rc_tip_text, rc_chkb_decline;
  PixelRect rc_prev_tip, rc_next_tip, rc_close;

  WndButton *prev_tip, *next_tip, *close;
  WndFrame *tip_text;
  CheckBoxControl *chkb_decline;

  StartupAssistant()
    :WndForm(UIGlobals::GetMainWindow(), UIGlobals::GetDialogLook(),
             UIGlobals::GetMainWindow().GetClientRect(),
             _T(""), GetDialogStyle())
  {}

  void OnTimer();

  virtual void Prepare(ContainerWindow &parent, const PixelRect &rc);
  virtual void Unprepare();
  virtual bool Save(bool &changed, bool &require_restart);
  virtual void Show(const PixelRect &rc);
  virtual void Hide() {};
  virtual void Move(const PixelRect &rc);

  /**
   * returns on full screen less height of the NavSliderWidget
   */
  virtual PixelRect GetSize(const PixelRect &rc);

  /**
   * returns height of TaskNavSlider bar
   */
  UPixelScalar GetNavSliderHeight();

  /**
   * from ActionListener
   */
  virtual void OnAction(int id);

  /**
   * sets the current tip and advances ui_settings.last_startup_tip
   * to the next tip in the array
   */
  void SetTip(bool forward);

  /**
   * sets up rectangles for layout of screen
   * @param rc. rect of dialog
   */
  void SetRectangles(const PixelRect &rc);
};

void
StartupAssistant::Move(const PixelRect &rc)
{
}

void
StartupAssistant::Show(const PixelRect &rc)
{
}

UPixelScalar
StartupAssistant::GetNavSliderHeight()
{
  UPixelScalar large_font_height = UIGlobals::GetLook().info_box.value.font->GetHeight();
  UPixelScalar small_font_height = UIGlobals::GetDialogLook().list.font->GetHeight();

  return large_font_height + 2 * small_font_height - Layout::Scale(9);
}

PixelRect
StartupAssistant::GetSize(const PixelRect &rc)
{
  UPixelScalar nav_slider_height = GetNavSliderHeight();

  PixelRect rc_form = rc;
  rc_form.left += Layout::landscape ? nav_slider_height / 2 :
      Layout::Scale(3);
  rc_form.right -= Layout::landscape ? nav_slider_height / 2 :
      Layout::Scale(3);
  rc_form.top = nav_slider_height + Layout::Scale(3);

  UPixelScalar height = min(rc.bottom - rc_form.top,
                            (PixelScalar)nav_slider_height * 6);
  rc_form.bottom = rc_form.top + height;

  return rc_form;
}

void
StartupAssistant::SetTip(bool forward)
{
  UISettings &ui_settings = CommonInterface::SetUISettings();
  assert(ARRAY_SIZE(tips) > 1);

  // 1-based index.  move to value between 1 and ARRAY_SIZE(tips)
  if (forward) {
    if (ui_settings.last_startup_tip < ARRAY_SIZE(tips))
      ui_settings.last_startup_tip++;
    else
      ui_settings.last_startup_tip = 1;
  } else {
    if (ui_settings.last_startup_tip > 1)
      ui_settings.last_startup_tip--;
    else
      ui_settings.last_startup_tip = ARRAY_SIZE(tips);
  }
  assert(ui_settings.last_startup_tip >= 1 &&
         ui_settings.last_startup_tip <= ARRAY_SIZE(tips));

  StaticString<512> tip;
  tip.Format(_T("Tip %u:\n\n%s"), ui_settings.last_startup_tip, tips[ui_settings.last_startup_tip - 1]);
  tip_text->SetCaption(tip.c_str());
}

void
StartupAssistant::OnAction(int id)
{
  if (id == NextTipClick)
    SetTip(true);

  else if (id == PrevTipClick)
    SetTip(false);

  else if (id == CloseDialogClick) {
    bool restart, changed;
    Save(restart, changed);
    SetModalResult(mrOK);
  }
}

void
StartupAssistant::OnTimer()
{
}

void
StartupAssistant::SetRectangles(const PixelRect &rc_outer)
{
  PixelRect rc;
  rc.left = rc.top = Layout::Scale(2);
  rc.right = rc_outer.right - rc_outer.left - Layout::Scale(2);
  rc.bottom = rc_outer.bottom - rc_outer.top - Layout::Scale(2);

  UPixelScalar button_height = GetNavSliderHeight();

  rc_close = rc_tip_text = rc_chkb_decline =
      rc_prev_tip = rc_next_tip = rc;
  rc_close.top = rc_close.bottom - button_height;

  rc_prev_tip.bottom = rc_next_tip.bottom =  rc_close.top - 1;
  rc_prev_tip.top = rc_next_tip.top = rc_next_tip.bottom -
      button_height;

  rc_prev_tip.right = (rc.right - rc.left) / 2;
  rc_next_tip.left = rc_prev_tip.right - 1;

  rc_chkb_decline.bottom = rc_prev_tip.top - 1;
  rc_chkb_decline.top = rc_chkb_decline.bottom - button_height / 2;
  rc_chkb_decline.right = rc_chkb_decline.left + (rc.right - rc.left); /*
      Layout::landscape ? 3 : 2;*/

  rc_tip_text.bottom = rc_chkb_decline.top - 1 - Layout::Scale(2);
  rc_tip_text.top = rc.top + Layout::Scale(2);
  rc_tip_text.left += Layout::Scale(2);
  rc_tip_text.right -= Layout::Scale(2);
}

void
StartupAssistant::Unprepare()
{
  delete prev_tip;
  delete next_tip;
  delete close;
  delete tip_text;
}

void
StartupAssistant::Prepare(ContainerWindow &parent, const PixelRect &rc)
{
  const PixelRect rc_form = GetSize(rc);
  NullWidget::Prepare(parent, rc_form);
  WndForm::Move(rc_form);

  SetRectangles(rc_form);

  WindowStyle style_frame;
  style_frame.Border();
  tip_text = new WndFrame(GetClientAreaWindow(), look,
                          rc_tip_text.left, rc_tip_text.top,
                          rc_tip_text.right - rc_tip_text.left,
                          rc_tip_text.bottom - rc_tip_text.top,
                          style_frame);
  tip_text->SetFont(Fonts::infobox_small);

  const DialogLook &dialog_look = UIGlobals::GetDialogLook();
  ButtonWindowStyle button_style;
  button_style.TabStop();
  button_style.multiline();
  close = new WndButton(GetClientAreaWindow(), dialog_look, _T("Close"),
                        rc_close,
                        button_style, this, CloseDialogClick);

  next_tip = new WndButton(GetClientAreaWindow(), dialog_look, _T("Next Tip"),
                           rc_next_tip,
                           button_style, this, NextTipClick);

  prev_tip = new WndButton(GetClientAreaWindow(), dialog_look, _T("Previous Tip"),
                           rc_prev_tip,
                           button_style, this, PrevTipClick);

  ButtonWindowStyle checkbox_style;
  checkbox_style.TabStop();
  chkb_decline = new CheckBoxControl(GetClientAreaWindow(),
                                     UIGlobals::GetDialogLook(),
                                     _("Don't show tips at startup"),
                                     rc_chkb_decline, checkbox_style,
                                     this, DeclineCheckboxClick);

  SetTip(true);
}

bool
StartupAssistant::Save(bool &changed, bool &require_restart)
{
  UISettings &ui_settings = CommonInterface::SetUISettings();
  Profile::Set(ProfileKeys::StartupTipId, ui_settings.last_startup_tip);
  bool declined = chkb_decline->GetState();
  Profile::Set(ProfileKeys::StartupTipDeclineVersion,
               declined ? TopHat_ProductToken : _T(""));

  Profile::Save();
  return true;
}

/**
 * if nationality is set to unknown, it prompts user if he wants to use
 * US settings
 */
static void
CheckContestNationality()
{
  ComputerSettings &settings_computer = XCSoarInterface::SetComputerSettings();
  TaskBehaviour &task_behaviour = settings_computer.task;

  if (task_behaviour.contest_nationality == ContestNationalities::UNKNOWN) {
    int result = ShowMessageBox(_(
        "Do you want Top Hat to use US task rules for flying tasks?"),
                                 _("Nationality settings"),
                                 MB_YESNO | MB_ICONQUESTION);
    if (result == IDYES)
      task_behaviour.contest_nationality = ContestNationalities::USA;
    else if (result == IDNO)
      task_behaviour.contest_nationality = ContestNationalities::ALL;
    else
      task_behaviour.contest_nationality = ContestNationalities::UNKNOWN;

    Profile::Set(ProfileKeys::ContestNationality,
                 (int)task_behaviour.contest_nationality);
    Profile::Save();
  }
}

/**
 * checks to see if a Plane, site files, and device are configured, and if no
 * displays the Quick Setup screen
 */
static bool
CheckConfigurationBasics()
{
  StaticString<255> text;

  const DeviceConfig &config =
    CommonInterface::SetSystemSettings().devices[0];
  const ComputerSettings &settings = CommonInterface::GetComputerSettings();
  const TaskBehaviour &task_behaviour = settings.task;

  if (config.driver_name.empty())
    return true;

  text.clear();
  if (Profile::Get(ProfileKeys::MapFile) != nullptr)
    text = Profile::Get(ProfileKeys::MapFile);

  if (text.empty() && Profile::Get(ProfileKeys::WaypointFile) != nullptr)
    text = Profile::Get(ProfileKeys::WaypointFile);
  if (text.empty())
    return true;

  text = settings.plane.registration;
  if (text.empty())
    return true;

  return false;
}

void
dlgStartupAssistantShowModal(bool conditional)
{
  CheckContestNationality();
  if (CheckConfigurationBasics())
    ShowDialogSetupQuick(true);

  StaticString<32> decline_ver;
  decline_ver = Profile::Get(ProfileKeys::StartupTipDeclineVersion, _T(""));
  if ((decline_ver == TopHat_ProductToken) && conditional)
    return;

  ContainerWindow &w = UIGlobals::GetMainWindow();
  StartupAssistant *instance = new StartupAssistant();
  instance->Initialise(w, instance->GetSize(w.GetClientRect()));
  instance->Prepare(w, instance->GetSize(w.GetClientRect()));
  instance->ShowModal();
  instance->Hide();
  instance->Unprepare();
  delete instance;
}
