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
DEFINE_bool  (wrap,    0,  "Wrap lines");
DEFINE_string(project, "", "CMake compile_commands.json");

BindMap *binds;
EditorDialog *editor; 

void MyReshaped() {
  editor->box = screen->Box();
  editor->Layout();
}
int Frame(LFL::Window *W, unsigned clicks, unsigned mic_samples, bool cam_sample, int flag) {
  screen->gd->DisableBlend();
  screen->DrawDialogs();
  return 0;
}

}; // namespace LFL
using namespace LFL;

extern "C" void LFAppCreateCB() {
  app->name = "LEdit";
  app->logfilename = StrCat(LFAppDownloadDir(), "ledit.txt");
  binds = new BindMap();
  screen->width = 840;
  screen->height = 760;
  screen->binds = binds;
  screen->caption = app->name;
  screen->frame_cb = Frame;
  FLAGS_lfapp_video = FLAGS_lfapp_input = true;
}

extern "C" int main(int argc, const char *argv[]) {
  if (app->Create(argc, argv, __FILE__, LFAppCreateCB)) { app->Free(); return -1; }
  if (app->Init()) { app->Free(); return -1; }
  app->scheduler.AddWaitForeverKeyboard();
  app->scheduler.AddWaitForeverMouse();
  app->reshaped_cb = MyReshaped;
  binds->Add(Bind('6', Key::Modifier::Cmd, Bind::CB(bind(&Shell::console, app->shell, vector<string>()))));

  chdir(app->startdir.c_str());
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
    (editor->editor.project = new IDE::Project())->LoadCMakeCompileCommandsJSON(&ccf);
  }

  app->scheduler.Start();
  return app->Main();
}
