/* Copyright 2011 Pierre Ossman <ossman@cendio.se> for Cendio AB
 * 
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this software; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
 * USA.
 */

#include <stdlib.h>

#include <list>

#include <FL/Fl_Tabs.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Return_Button.H>

#include <rdr/types.h>
#include <rfb/encodings.h>

#ifdef HAVE_GNUTLS
#include <rfb/Security.h>
#include <rfb/SecurityClient.h>
#include <rfb/CSecurityTLS.h>
#endif

#include "OptionsDialog.h"
#include "fltk_layout.h"
#include "i18n.h"
#include "parameters.h"

using namespace std;
using namespace rdr;
using namespace rfb;

std::map<OptionsCallback*, void*> OptionsDialog::callbacks;

OptionsDialog::OptionsDialog()
  : Fl_Window(450, 450, _("VNC Viewer: Connection Options"))
{
  int x, y;
  Fl_Button *button;

  Fl_Tabs *tabs = new Fl_Tabs(OUTER_MARGIN, OUTER_MARGIN,
                             w() - OUTER_MARGIN*2,
                             h() - OUTER_MARGIN*2 - INNER_MARGIN - BUTTON_HEIGHT);

  {
    int tx, ty, tw, th;

    tabs->client_area(tx, ty, tw, th, TABS_HEIGHT);

    createCompressionPage(tx, ty, tw, th);
    createSecurityPage(tx, ty, tw, th);
    createInputPage(tx, ty, tw, th);
    createMiscPage(tx, ty, tw, th);
  }

  tabs->end();

  x = w() - BUTTON_WIDTH * 2 - INNER_MARGIN - OUTER_MARGIN;
  y = h() - BUTTON_HEIGHT - OUTER_MARGIN;

  button = new Fl_Button(x, y, BUTTON_WIDTH, BUTTON_HEIGHT, _("Cancel"));
  button->callback(this->handleCancel, this);

  x += BUTTON_WIDTH + INNER_MARGIN;

  button = new Fl_Return_Button(x, y, BUTTON_WIDTH, BUTTON_HEIGHT, _("OK"));
  button->callback(this->handleOK, this);

  callback(this->handleCancel, this);

  set_modal();
}


OptionsDialog::~OptionsDialog()
{
}


void OptionsDialog::showDialog(void)
{
  static OptionsDialog *dialog = NULL;

  if (!dialog)
    dialog = new OptionsDialog();

  if (dialog->shown())
    return;

  dialog->show();
}


void OptionsDialog::addCallback(OptionsCallback *cb, void *data)
{
  callbacks[cb] = data;
}


void OptionsDialog::removeCallback(OptionsCallback *cb)
{
  callbacks.erase(cb);
}


void OptionsDialog::show(void)
{
  loadOptions();

  Fl_Window::show();
}


void OptionsDialog::loadOptions(void)
{
  /* Compression */
  autoselectCheckbox->value(autoSelect);

  int encNum = encodingNum(preferredEncoding);

  switch (encNum) {
  case encodingTight:
    tightButton->setonly();
    break;
  case encodingZRLE:
    zrleButton->setonly();
    break;
  case encodingHextile:
    hextileButton->setonly();
    break;
  case encodingRaw:
    rawButton->setonly();
    break;
  }

  if (fullColour)
    fullcolorCheckbox->setonly();
  else {
    switch (lowColourLevel) {
    case 0:
      mediumcolorCheckbox->setonly();
      break;
    case 1:
      lowcolorCheckbox->setonly();
      break;
    case 2:
      verylowcolorCheckbox->setonly();
      break;
    }
  }

  char digit[2] = "0";

  compressionCheckbox->value(customCompressLevel);
  jpegCheckbox->value(!noJpeg);
  digit[0] = '0' + compressLevel;
  compressionInput->value(digit);
  digit[0] = '0' + qualityLevel;
  jpegInput->value(digit);

  handleAutoselect(autoselectCheckbox, this);
  handleCompression(compressionCheckbox, this);
  handleJpeg(jpegCheckbox, this);

#ifdef HAVE_GNUTLS
  /* Security */
  Security security(SecurityClient::secTypes);

  list<U8> secTypes;
  list<U8>::iterator iter;

   list<U32> secTypesExt;
   list<U32>::iterator iterExt;

  vencryptCheckbox->value(false);

  encNoneCheckbox->value(false);
  encTLSCheckbox->value(false);
  encX509Checkbox->value(false);

  authNoneCheckbox->value(false);
  authVncCheckbox->value(false);
  authPlainCheckbox->value(false);

  secTypes = security.GetEnabledSecTypes();
  for (iter = secTypes.begin(); iter != secTypes.end(); ++iter) {
    switch (*iter) {
    case secTypeVeNCrypt:
      vencryptCheckbox->value(true);
      break;
    case secTypeNone:
      encNoneCheckbox->value(true);
      authNoneCheckbox->value(true);
      break;
    case secTypeVncAuth:
      encNoneCheckbox->value(true);
      authVncCheckbox->value(true);
      break;
    }
  }

  secTypesExt = security.GetEnabledExtSecTypes();
  for (iterExt = secTypesExt.begin(); iterExt != secTypesExt.end(); ++iterExt) {
    switch (*iterExt) {
    case secTypePlain:
      encNoneCheckbox->value(true);
      authPlainCheckbox->value(true);
      break;
    case secTypeTLSNone:
      encTLSCheckbox->value(true);
      authNoneCheckbox->value(true);
      break;
    case secTypeTLSVnc:
      encTLSCheckbox->value(true);
      authVncCheckbox->value(true);
      break;
    case secTypeTLSPlain:
      encTLSCheckbox->value(true);
      authPlainCheckbox->value(true);
      break;
    case secTypeX509None:
      encX509Checkbox->value(true);
      authNoneCheckbox->value(true);
      break;
    case secTypeX509Vnc:
      encX509Checkbox->value(true);
      authVncCheckbox->value(true);
      break;
    case secTypeX509Plain:
      encX509Checkbox->value(true);
      authPlainCheckbox->value(true);
      break;
    }
  }

  caInput->value(CSecurityTLS::x509ca);
  crlInput->value(CSecurityTLS::x509crl);

  handleVencrypt(vencryptCheckbox, this);
  handleX509(encX509Checkbox, this);
#endif

  /* Input */
  const char *menuKeyBuf;

  viewOnlyCheckbox->value(viewOnly);
  acceptClipboardCheckbox->value(acceptClipboard);
  sendClipboardCheckbox->value(sendClipboard);
  sendPrimaryCheckbox->value(sendPrimary);
  systemKeysCheckbox->value(fullscreenSystemKeys);

  menuKeyChoice->value(0);

  menuKeyBuf = menuKey;
  if (menuKeyBuf[0] == 'F') {
    int num = atoi(menuKeyBuf+1);
    if ((num >= 1) && (num <= 12))
      menuKeyChoice->value(num);
  }

  /* Misc. */
  sharedCheckbox->value(shared);
  fullScreenCheckbox->value(fullScreen);
  dotCursorCheckbox->value(dotWhenNoCursor);
}


void OptionsDialog::storeOptions(void)
{
  /* Compression */
  autoSelect.setParam(autoselectCheckbox->value());

  if (tightButton->value())
    preferredEncoding.setParam(encodingName(encodingTight));
  else if (zrleButton->value())
    preferredEncoding.setParam(encodingName(encodingZRLE));
  else if (hextileButton->value())
    preferredEncoding.setParam(encodingName(encodingHextile));
  else if (rawButton->value())
    preferredEncoding.setParam(encodingName(encodingRaw));

  fullColour.setParam(fullcolorCheckbox->value());
  if (mediumcolorCheckbox->value())
    lowColourLevel.setParam(0);
  else if (lowcolorCheckbox->value())
    lowColourLevel.setParam(1);
  else if (verylowcolorCheckbox->value())
    lowColourLevel.setParam(2);

  customCompressLevel.setParam(compressionCheckbox->value());
  noJpeg.setParam(!jpegCheckbox->value());
  compressLevel.setParam(atoi(compressionInput->value()));
  qualityLevel.setParam(atoi(jpegInput->value()));

#ifdef HAVE_GNUTLS
  /* Security */
  Security security;

  /* Process security types which don't use encryption */
  if (encNoneCheckbox->value()) {
    if (authNoneCheckbox->value())
      security.EnableSecType(secTypeNone);
    if (authVncCheckbox->value())
      security.EnableSecType(secTypeVncAuth);

    if (vencryptCheckbox->value()) {
      if (authPlainCheckbox->value())
        security.EnableSecType(secTypePlain);
    }
  }

  if (vencryptCheckbox->value()) {
    /* Process security types which use TLS encryption */
    if (encTLSCheckbox->value()) {
      if (authNoneCheckbox->value())
        security.EnableSecType(secTypeTLSNone);
      if (authVncCheckbox->value())
        security.EnableSecType(secTypeTLSVnc);
      if (authPlainCheckbox->value())
        security.EnableSecType(secTypeTLSPlain);
    }

    /* Process security types which use X509 encryption */
    if (encX509Checkbox->value()) {
      if (authNoneCheckbox->value())
        security.EnableSecType(secTypeX509None);
      if (authVncCheckbox->value())
        security.EnableSecType(secTypeX509Vnc);
      if (authPlainCheckbox->value())
        security.EnableSecType(secTypeX509Plain);
    }
  }

  CSecurityTLS::x509ca.setParam(caInput->value());
  CSecurityTLS::x509crl.setParam(crlInput->value());
#endif

  /* Input */
  viewOnly.setParam(viewOnlyCheckbox->value());
  acceptClipboard.setParam(acceptClipboardCheckbox->value());
  sendClipboard.setParam(sendClipboardCheckbox->value());
  sendPrimary.setParam(sendPrimaryCheckbox->value());
  fullscreenSystemKeys.setParam(systemKeysCheckbox->value());

  if (menuKeyChoice->value() == 0)
    menuKey.setParam("");
  else {
    char buf[16];
    sprintf(buf, "F%d", menuKeyChoice->value());
    menuKey.setParam(buf);
  }

  /* Misc. */
  shared.setParam(sharedCheckbox->value());
  fullScreen.setParam(fullScreenCheckbox->value());
  dotWhenNoCursor.setParam(dotCursorCheckbox->value());

  std::map<OptionsCallback*, void*>::const_iterator iter;

  for (iter = callbacks.begin();iter != callbacks.end();++iter)
    iter->first(iter->second);
}


void OptionsDialog::createCompressionPage(int tx, int ty, int tw, int th)
{
  Fl_Group *group = new Fl_Group(tx, ty, tw, th, _("Compression"));

  int orig_tx, orig_ty;
  int half_width, full_width;
  int width, height;

  tx += OUTER_MARGIN;
  ty += OUTER_MARGIN;

  full_width = tw - OUTER_MARGIN * 2;
  half_width = (full_width - INNER_MARGIN) / 2;

  /* AutoSelect checkbox */
  autoselectCheckbox = new Fl_Check_Button(LBLRIGHT(tx, ty,
                                                     CHECK_MIN_WIDTH,
                                                     CHECK_HEIGHT,
                                                     _("Auto select")));
  autoselectCheckbox->callback(handleAutoselect, this);
  ty += CHECK_HEIGHT + INNER_MARGIN;

  /* Two columns */
  orig_tx = tx;
  orig_ty = ty;

  /* VNC encoding box */
  ty += GROUP_LABEL_OFFSET;
  height = GROUP_MARGIN * 2 + TIGHT_MARGIN * 3 + RADIO_HEIGHT * 4;
  encodingGroup = new Fl_Group(tx, ty, half_width, height,
                                _("Preferred encoding"));
  encodingGroup->box(FL_ENGRAVED_BOX);
  encodingGroup->align(FL_ALIGN_LEFT | FL_ALIGN_TOP);

  {
    tx += GROUP_MARGIN;
    ty += GROUP_MARGIN;

    width = encodingGroup->w() - GROUP_MARGIN * 2;

    tightButton = new Fl_Round_Button(LBLRIGHT(tx, ty,
                                               RADIO_MIN_WIDTH,
                                               RADIO_HEIGHT,
                                               _("Tight")));
    tightButton->type(FL_RADIO_BUTTON);
    ty += RADIO_HEIGHT + TIGHT_MARGIN;

    zrleButton = new Fl_Round_Button(LBLRIGHT(tx, ty,
                                              RADIO_MIN_WIDTH,
                                              RADIO_HEIGHT,
                                              _("ZRLE")));
    zrleButton->type(FL_RADIO_BUTTON);
    ty += RADIO_HEIGHT + TIGHT_MARGIN;

    hextileButton = new Fl_Round_Button(LBLRIGHT(tx, ty,
                                                 RADIO_MIN_WIDTH,
                                                 RADIO_HEIGHT,
                                                 _("Hextile")));
    hextileButton->type(FL_RADIO_BUTTON);
    ty += RADIO_HEIGHT + TIGHT_MARGIN;

    rawButton = new Fl_Round_Button(LBLRIGHT(tx, ty,
                                             RADIO_MIN_WIDTH,
                                             RADIO_HEIGHT,
                                             _("Raw")));
    rawButton->type(FL_RADIO_BUTTON);
    ty += RADIO_HEIGHT + TIGHT_MARGIN;
  }

  ty += GROUP_MARGIN - TIGHT_MARGIN;

  encodingGroup->end();

  /* Second column */
  tx = orig_tx + half_width + INNER_MARGIN;
  ty = orig_ty;

  /* Color box */
  ty += GROUP_LABEL_OFFSET;
  height = GROUP_MARGIN * 2 + TIGHT_MARGIN * 3 + RADIO_HEIGHT * 4;
  colorlevelGroup = new Fl_Group(tx, ty, half_width, height, _("Color level"));
  colorlevelGroup->box(FL_ENGRAVED_BOX);
  colorlevelGroup->align(FL_ALIGN_LEFT | FL_ALIGN_TOP);

  {
    tx += GROUP_MARGIN;
    ty += GROUP_MARGIN;

    width = colorlevelGroup->w() - GROUP_MARGIN * 2;

    fullcolorCheckbox = new Fl_Round_Button(LBLRIGHT(tx, ty,
                                                     RADIO_MIN_WIDTH,
                                                     RADIO_HEIGHT,
                                                     _("Full (all available colors)")));
    fullcolorCheckbox->type(FL_RADIO_BUTTON);
    ty += RADIO_HEIGHT + TIGHT_MARGIN;

    mediumcolorCheckbox = new Fl_Round_Button(LBLRIGHT(tx, ty,
                                                       RADIO_MIN_WIDTH,
                                                       RADIO_HEIGHT,
                                                       _("Medium (256 colors)")));
    mediumcolorCheckbox->type(FL_RADIO_BUTTON);
    ty += RADIO_HEIGHT + TIGHT_MARGIN;

    lowcolorCheckbox = new Fl_Round_Button(LBLRIGHT(tx, ty,
                                                    RADIO_MIN_WIDTH,
                                                    RADIO_HEIGHT,
                                                    _("Low (64 colors)")));
    lowcolorCheckbox->type(FL_RADIO_BUTTON);
    ty += RADIO_HEIGHT + TIGHT_MARGIN;

    verylowcolorCheckbox = new Fl_Round_Button(LBLRIGHT(tx, ty,
                                                        RADIO_MIN_WIDTH,
                                                        RADIO_HEIGHT,
                                                        _("Very low (8 colors)")));
    verylowcolorCheckbox->type(FL_RADIO_BUTTON);
    ty += RADIO_HEIGHT + TIGHT_MARGIN;
  }

  ty += GROUP_MARGIN - TIGHT_MARGIN;

  colorlevelGroup->end();

  /* Back to normal */
  tx = orig_tx;
  ty += INNER_MARGIN;

  /* Checkboxes */
  compressionCheckbox = new Fl_Check_Button(LBLRIGHT(tx, ty,
                                                     CHECK_MIN_WIDTH,
                                                     CHECK_HEIGHT,
                                                     _("Custom compression level:")));
  compressionCheckbox->callback(handleCompression, this);
  ty += CHECK_HEIGHT + TIGHT_MARGIN;

  compressionInput = new Fl_Int_Input(tx + INDENT, ty,
                                      INPUT_HEIGHT, INPUT_HEIGHT,
                                      _("level (1=fast, 9=best)"));
  compressionInput->align(FL_ALIGN_RIGHT);
  ty += INPUT_HEIGHT + INNER_MARGIN;

  jpegCheckbox = new Fl_Check_Button(LBLRIGHT(tx, ty,
                                              CHECK_MIN_WIDTH,
                                              CHECK_HEIGHT,
                                              _("Allow JPEG compression:")));
  jpegCheckbox->callback(handleJpeg, this);
  ty += CHECK_HEIGHT + TIGHT_MARGIN;

  jpegInput = new Fl_Int_Input(tx + INDENT, ty,
                               INPUT_HEIGHT, INPUT_HEIGHT,
                               _("quality (1=poor, 9=best)"));
  jpegInput->align(FL_ALIGN_RIGHT);
  ty += INPUT_HEIGHT + INNER_MARGIN;

  group->end();
}


void OptionsDialog::createSecurityPage(int tx, int ty, int tw, int th)
{
#ifdef HAVE_GNUTLS
  Fl_Group *group = new Fl_Group(tx, ty, tw, th, _("Security"));

  int orig_tx;
  int width, height;

  tx += OUTER_MARGIN;
  ty += OUTER_MARGIN;

  width = tw - OUTER_MARGIN * 2;

  orig_tx = tx;

  /* Security */
  vencryptCheckbox = new Fl_Check_Button(LBLRIGHT(tx, ty,
                                                  CHECK_MIN_WIDTH,
                                                  CHECK_HEIGHT,
                                                  _("Extended encryption and authentication methods (VeNCrypt)")));
  vencryptCheckbox->callback(handleVencrypt, this);
  ty += CHECK_HEIGHT + INNER_MARGIN;

  /* Encryption */
  ty += GROUP_LABEL_OFFSET;
  height = GROUP_MARGIN * 2 + TIGHT_MARGIN * 4 + CHECK_HEIGHT * 3 + (INPUT_LABEL_OFFSET + INPUT_HEIGHT) * 2;
  encryptionGroup = new Fl_Group(tx, ty, width, height, _("Encryption"));
  encryptionGroup->box(FL_ENGRAVED_BOX);
  encryptionGroup->align(FL_ALIGN_LEFT | FL_ALIGN_TOP);

  {
    tx += GROUP_MARGIN;
    ty += GROUP_MARGIN;

    encNoneCheckbox = new Fl_Check_Button(LBLRIGHT(tx, ty,
                                                   CHECK_MIN_WIDTH,
                                                   CHECK_HEIGHT,
                                                   _("None")));
    ty += CHECK_HEIGHT + TIGHT_MARGIN;

    encTLSCheckbox = new Fl_Check_Button(LBLRIGHT(tx, ty,
                                                  CHECK_MIN_WIDTH,
                                                  CHECK_HEIGHT,
                                                  _("TLS with anonymous certificates")));
    ty += CHECK_HEIGHT + TIGHT_MARGIN;

    encX509Checkbox = new Fl_Check_Button(LBLRIGHT(tx, ty,
                                                   CHECK_MIN_WIDTH,
                                                   CHECK_HEIGHT,
                                                   _("TLS with X509 certificates")));
    encX509Checkbox->callback(handleX509, this);
    ty += CHECK_HEIGHT + TIGHT_MARGIN;

    ty += INPUT_LABEL_OFFSET;
    caInput = new Fl_Input(tx + INDENT, ty, 
                           width - GROUP_MARGIN*2 - INDENT, INPUT_HEIGHT,
                           _("Path to X509 CA certificate"));
    caInput->align(FL_ALIGN_LEFT | FL_ALIGN_TOP);
    ty += INPUT_HEIGHT + TIGHT_MARGIN;

    ty += INPUT_LABEL_OFFSET;
    crlInput = new Fl_Input(tx + INDENT, ty,
                            width - GROUP_MARGIN*2 - INDENT, INPUT_HEIGHT,
                            _("Path to X509 CRL file"));
    crlInput->align(FL_ALIGN_LEFT | FL_ALIGN_TOP);
    ty += INPUT_HEIGHT + TIGHT_MARGIN;
  }

  ty += GROUP_MARGIN - TIGHT_MARGIN;

  encryptionGroup->end();

  /* Back to normal */
  tx = orig_tx;
  ty += INNER_MARGIN;

  /* Authentication */
  /* Encryption */
  ty += GROUP_LABEL_OFFSET;
  height = GROUP_MARGIN * 2 + TIGHT_MARGIN * 2 + CHECK_HEIGHT * 3;
  authenticationGroup = new Fl_Group(tx, ty, width, height, _("Authentication"));
  authenticationGroup->box(FL_ENGRAVED_BOX);
  authenticationGroup->align(FL_ALIGN_LEFT | FL_ALIGN_TOP);

  {
    tx += GROUP_MARGIN;
    ty += GROUP_MARGIN;

    authNoneCheckbox = new Fl_Check_Button(LBLRIGHT(tx, ty,
                                                    CHECK_MIN_WIDTH,
                                                    CHECK_HEIGHT,
                                                    _("None")));
    ty += CHECK_HEIGHT + TIGHT_MARGIN;

    authVncCheckbox = new Fl_Check_Button(LBLRIGHT(tx, ty,
                                                   CHECK_MIN_WIDTH,
                                                   CHECK_HEIGHT,
                                                   _("Standard VNC (insecure without encryption)")));
    ty += CHECK_HEIGHT + TIGHT_MARGIN;

    authPlainCheckbox = new Fl_Check_Button(LBLRIGHT(tx, ty,
                                                     CHECK_MIN_WIDTH,
                                                     CHECK_HEIGHT,
                                                     _("Username and password (insecure without encryption)")));
    ty += CHECK_HEIGHT + TIGHT_MARGIN;
  }

  ty += GROUP_MARGIN - TIGHT_MARGIN;

  authenticationGroup->end();

  /* Back to normal */
  tx = orig_tx;
  ty += INNER_MARGIN;

  group->end();
#endif
}


void OptionsDialog::createInputPage(int tx, int ty, int tw, int th)
{
  Fl_Group *group = new Fl_Group(tx, ty, tw, th, _("Input"));

  tx += OUTER_MARGIN;
  ty += OUTER_MARGIN;

  viewOnlyCheckbox = new Fl_Check_Button(LBLRIGHT(tx, ty,
                                                  CHECK_MIN_WIDTH,
                                                  CHECK_HEIGHT,
                                                  _("View only (ignore mouse and keyboard)")));
  ty += CHECK_HEIGHT + TIGHT_MARGIN;

  acceptClipboardCheckbox = new Fl_Check_Button(LBLRIGHT(tx, ty,
                                                         CHECK_MIN_WIDTH,
                                                         CHECK_HEIGHT,
                                                         _("Accept clipboard from server")));
  ty += CHECK_HEIGHT + TIGHT_MARGIN;

  sendClipboardCheckbox = new Fl_Check_Button(LBLRIGHT(tx, ty,
                                                       CHECK_MIN_WIDTH,
                                                       CHECK_HEIGHT,
                                                       _("Send clipboard to server")));
  ty += CHECK_HEIGHT + TIGHT_MARGIN;

  sendPrimaryCheckbox = new Fl_Check_Button(LBLRIGHT(tx, ty,
                                                     CHECK_MIN_WIDTH,
                                                     CHECK_HEIGHT,
                                                     _("Send primary selection and cut buffer as clipboard")));
  ty += CHECK_HEIGHT + TIGHT_MARGIN;

  systemKeysCheckbox = new Fl_Check_Button(LBLRIGHT(tx, ty,
                                                    CHECK_MIN_WIDTH,
                                                    CHECK_HEIGHT,
                                                    _("Pass system keys directly to server (full screen)")));
  ty += CHECK_HEIGHT + TIGHT_MARGIN;

  menuKeyChoice = new Fl_Choice(LBLLEFT(tx, ty, 150, CHOICE_HEIGHT, _("Menu key")));

  menuKeyChoice->add(_("None"), 0, NULL, (void*)0, FL_MENU_DIVIDER);
  for (int i = 1;i <= 12;i++) {
    char buf[16];
    sprintf(buf, "F%d", i);
    menuKeyChoice->add(buf, 0, NULL, (void*)i, 0);
  }

  ty += CHOICE_HEIGHT + TIGHT_MARGIN;

  group->end();
}


void OptionsDialog::createMiscPage(int tx, int ty, int tw, int th)
{
  Fl_Group *group = new Fl_Group(tx, ty, tw, th, _("Misc."));

  tx += OUTER_MARGIN;
  ty += OUTER_MARGIN;

  sharedCheckbox = new Fl_Check_Button(LBLRIGHT(tx, ty,
                                                  CHECK_MIN_WIDTH,
                                                  CHECK_HEIGHT,
                                                  _("Shared (don't disconnect other viewers)")));
  ty += CHECK_HEIGHT + TIGHT_MARGIN;

  fullScreenCheckbox = new Fl_Check_Button(LBLRIGHT(tx, ty,
                                                  CHECK_MIN_WIDTH,
                                                  CHECK_HEIGHT,
                                                  _("Full-screen mode")));
  ty += CHECK_HEIGHT + TIGHT_MARGIN;

  dotCursorCheckbox = new Fl_Check_Button(LBLRIGHT(tx, ty,
                                                  CHECK_MIN_WIDTH,
                                                  CHECK_HEIGHT,
                                                  _("Show dot when no cursor")));
  ty += CHECK_HEIGHT + TIGHT_MARGIN;

  group->end();
}


void OptionsDialog::handleAutoselect(Fl_Widget *widget, void *data)
{
  OptionsDialog *dialog = (OptionsDialog*)data;

  if (dialog->autoselectCheckbox->value()) {
    dialog->encodingGroup->deactivate();
    dialog->colorlevelGroup->deactivate();
  } else {
    dialog->encodingGroup->activate();
    dialog->colorlevelGroup->activate();
  }

  // JPEG setting is also affected by autoselection
  dialog->handleJpeg(dialog->jpegCheckbox, dialog);
}


void OptionsDialog::handleCompression(Fl_Widget *widget, void *data)
{
  OptionsDialog *dialog = (OptionsDialog*)data;

  if (dialog->compressionCheckbox->value())
    dialog->compressionInput->activate();
  else
    dialog->compressionInput->deactivate();
}


void OptionsDialog::handleJpeg(Fl_Widget *widget, void *data)
{
  OptionsDialog *dialog = (OptionsDialog*)data;

  if (dialog->jpegCheckbox->value() &&
      !dialog->autoselectCheckbox->value())
    dialog->jpegInput->activate();
  else
    dialog->jpegInput->deactivate();
}


void OptionsDialog::handleVencrypt(Fl_Widget *widget, void *data)
{
  OptionsDialog *dialog = (OptionsDialog*)data;

  if (dialog->vencryptCheckbox->value()) {
    dialog->encryptionGroup->activate();
    dialog->authPlainCheckbox->activate();
  } else {
    dialog->encryptionGroup->deactivate();
    dialog->authPlainCheckbox->deactivate();
  }
}


void OptionsDialog::handleX509(Fl_Widget *widget, void *data)
{
  OptionsDialog *dialog = (OptionsDialog*)data;

  if (dialog->encX509Checkbox->value()) {
    dialog->caInput->activate();
    dialog->crlInput->activate();
  } else {
    dialog->caInput->deactivate();
    dialog->crlInput->deactivate();
  }
}


void OptionsDialog::handleCancel(Fl_Widget *widget, void *data)
{
  OptionsDialog *dialog = (OptionsDialog*)data;

  dialog->hide();
}


void OptionsDialog::handleOK(Fl_Widget *widget, void *data)
{
  OptionsDialog *dialog = (OptionsDialog*)data;

  dialog->hide();

  dialog->storeOptions();
}