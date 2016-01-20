/*
 * $Id$
 * Copyright (C) 2009 Lucid Fusion Labs

 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "lfapp/lfapp.h"
#include "lfapp/dom.h"
#include "lfapp/css.h"
#include "lfapp/flow.h"
#include "lfapp/gui.h"

namespace LFL {
DEFINE_bool  (wrap,    0,   "Wrap lines");
DEFINE_string(project, "",  "CMake compile_commands.json");
DEFINE_int   (width,   840, "Window width");
DEFINE_int   (height,  760, "Window height");

BindMap *binds;
EditorDialog *editor; 
bool console_animating = 0;
vector<MenuItem> context_menu{ MenuItem{ "", "Go To Definition", "gotodef" } };

void MyReshaped() {
  editor->box = screen->Box();
  editor->Layout();
}

int Frame(LFL::Window *W, unsigned clicks, int flag) {
  screen->gd->DisableBlend();
  screen->DrawDialogs();
  return 0;
}

void UpdateAnimating() { app->scheduler.SetAnimating(console_animating); }
void OnConsoleAnimating() { console_animating = screen->lfapp_console->animating; UpdateAnimating(); }

}; // namespace LFL
using namespace LFL;

extern "C" void LFAppCreateCB() {
  app->name = "LEdit";
  app->logfilename = StrCat(LFAppDownloadDir(), "ledit.txt");
  binds = new BindMap();
  screen->width = FLAGS_width;
  screen->height = FLAGS_height;
  screen->binds = binds;
  screen->caption = app->name;
  screen->frame_cb = Frame;
  FLAGS_lfapp_video = FLAGS_lfapp_input = true;
}

extern "C" int main(int argc, const char *argv[]) {
  if (app->Create(argc, argv, __FILE__, LFAppCreateCB)) { app->Free(); return -1; }
  screen->width = FLAGS_width;
  screen->height = FLAGS_height;

  if (app->Init()) { app->Free(); return -1; }
  app->scheduler.AddWaitForeverKeyboard();
  app->scheduler.AddWaitForeverMouse();
  app->reshaped_cb = MyReshaped;
  app->shell.command.push_back(Shell::Command("save",    [](const vector<string>&){ editor->editor.Save();                        app->scheduler.Wakeup(0); }));
  app->shell.command.push_back(Shell::Command("wrap",    [](const vector<string>&){ editor->editor.ToggleShouldWrap();            app->scheduler.Wakeup(0); }));
  app->shell.command.push_back(Shell::Command("gotodef", [](const vector<string>&){ editor->editor.GoToDefinition(screen->mouse); app->scheduler.Wakeup(0); }));
  if (screen->lfapp_console) screen->lfapp_console->animating_cb = OnConsoleAnimating;

  binds->Add(Bind('6', Key::Modifier::Cmd, Bind::CB(bind(&Shell::console, app->shell, vector<string>()))));
  binds->Add(Bind('s', Key::Modifier::Cmd, Bind::CB([=](){ app->shell.console(vector<string>(1, "save")); })));
  binds->Add(Bind('w', Key::Modifier::Cmd, Bind::CB([=](){ app->shell.console(vector<string>(1, "wrap")); })));
  binds->Add(Bind(Mouse::Button::_1, Key::Modifier::Ctrl, Bind::CB([=](){ app->LaunchNativeContextMenu(context_menu); })));

  vector<MenuItem> file_menu{ MenuItem{ "s", "Save", "save" }, };
  app->AddNativeMenu("File", file_menu);
  app->AddNativeEditMenu();

  vector<MenuItem> view_menu{
    MenuItem{ "=", "Zoom In", "" }, MenuItem{ "-", "Zoom Out", "" },
    MenuItem{ "w", "Wrap lines", "wrap" } };
  app->AddNativeMenu("View", view_menu);

  int optind = Singleton<FlagMap>::Get()->optind;
  if (optind >= argc) { fprintf(stderr, "Usage: %s [-flags] <file>\n", argv[0]); return -1; }

  Font *font = Fonts::Get(FLAGS_default_font, "", FLAGS_default_font_size, Color::black, Color::white, FLAGS_default_font_flag);
  editor = new EditorDialog(screen, font, new LocalFile(argv[optind], "r"), 1, 1,
                            Dialog::Flag::Fullscreen | (FLAGS_wrap ? EditorDialog::Flag::Wrap : 0));
  editor->editor.line.SetAttrSource(&editor->editor);
  editor->editor.SetColors(Singleton<TextGUI::SolarizedLightColors>::Get());
  if (auto c = editor->editor.bg_color) screen->gd->ClearColor((editor->color = *c));
  if (FLAGS_project.size()) {
    LocalFile ccf(FLAGS_project, "r");
    (editor->editor.project = new IDE::Project())->LoadCMakeCompileCommandsFile(&ccf);
  }
  screen->default_textgui = &editor->editor;
  screen->AddDialog(editor);

  return app->Main();
}
