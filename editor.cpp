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

#include "core/app/app.h"
#include "core/web/dom.h"
#include "core/web/css.h"
#include "core/app/flow.h"
#include "core/app/gui.h"

namespace LFL {
DEFINE_string(project, "",  "CMake compile_commands.json");
DEFINE_int   (width,   840, "Window width");
DEFINE_int   (height,  760, "Window height");

struct EditorGUI : public GUI {
  TabbedDialog<EditorDialog> tabs;
  unique_ptr<IDE::Project> project;
  vector<MenuItem> context_menu{ MenuItem{ "", "Go To Definition", "gotodef" } };
  bool console_animating = 0;

  EditorGUI() : tabs(this) { Activate(); }
  Editor *Top() { return tabs.top ? &tabs.top->editor : 0; }

  void GotoDefinition() {
    FileNameAndOffset fo = Top() ? Top()->FindDefinition(screen->mouse) : FileNameAndOffset();
    if (fo.fn.empty()) return;
    INFO("Editor GotoDefinition ", fo.fn, " ", fo.offset, " ", fo.y, " ", fo.x);
    auto editor = Open(fo.fn);
    editor->editor.UpdateWrappedLines(editor->editor.font->size, editor->content.w);
    editor->editor.UpdateAnnotation();
    int lines = tabs.box.h / editor->editor.font->Height();
    editor->editor.SetVScroll(fo.y - lines/2);
    editor->editor.cursor.i.y = lines/2 - 1;
    editor->editor.UpdateCursorX(fo.x);
  }

  EditorDialog *Open(const string &fin) {
    static string prefix = "file://";
    string fn = PrefixMatch(fin, prefix) ? fin.substr(prefix.size()) : fin;
    INFO("Editor Open ", fn);
    Font *font = app->fonts->Get(FLAGS_default_font, "", FLAGS_default_font_size, Color::black, Color::white, FLAGS_default_font_flag);
    EditorDialog *editor = new EditorDialog(screen->gd, font, new LocalFile(fn, "r"), 1, 1); 
    editor->editor.project = project.get();
    editor->editor.line.SetAttrSource(&editor->editor);
    editor->editor.SetColors(Singleton<TextBox::SolarizedLightColors>::Get());
    editor->deleted_cb = [=](){ tabs.DelTab(editor); child_box.Clear(); delete editor; };
    if (auto c = editor->editor.bg_color) screen->gd->ClearColor((editor->color = *c));
    tabs.AddTab(editor);
    child_box.Clear();
    return editor;
  }

  void Layout() {
    Reset();
    box = tabs.box = screen->Box();
    tabs.Layout();
    if (!child_box.Size()) child_box.PushNop();
  }

  int Frame(LFL::Window *W, unsigned clicks, int flag) {
    screen->gd->DisableBlend();
    GUI::Draw();
    tabs.Draw();
    screen->DrawDialogs();
    return 0;
  }

  void UpdateAnimating() { app->scheduler.SetAnimating(console_animating); }
  void OnConsoleAnimating() { console_animating = screen->console->animating; UpdateAnimating(); }
};

void MyWindowInit(Window *W) {
  screen->width = FLAGS_width;
  screen->height = FLAGS_height;
  screen->caption = app->name;
}

void MyWindowStart(Window *W) {
  EditorGUI *editor_gui = W->AddGUI(make_unique<EditorGUI>());
  if (FLAGS_console) W->InitConsole(bind(&EditorGUI::OnConsoleAnimating, editor_gui));
  W->frame_cb = bind(&EditorGUI::Frame, editor_gui, _1, _2, _3);
  W->default_textbox = [=](){ return editor_gui->Top(); };

  BindMap *binds = W->AddInputController(make_unique<BindMap>());
  binds->Add('6', Key::Modifier::Cmd, Bind::CB(bind(&Shell::console, W->shell.get(), vector<string>())));
  binds->Add('o', Key::Modifier::Cmd, Bind::CB([=](){ W->shell->console(vector<string>(1, "choose")); }));
  binds->Add('s', Key::Modifier::Cmd, Bind::CB([=](){ W->shell->console(vector<string>(1, "save")); }));
  binds->Add('w', Key::Modifier::Cmd, Bind::CB([=](){ W->shell->console(vector<string>(1, "wrap")); }));
  binds->Add(Mouse::Button::_1, Key::Modifier::Ctrl, Bind::CB([=](){ app->LaunchNativeContextMenu(editor_gui->context_menu); }));

  W->shell = make_unique<Shell>(nullptr, nullptr, nullptr);
  W->shell->Add("choose",  [=](const vector<string>&) { app->LaunchNativeFileChooser(1,0,0,"open"); });
  W->shell->Add("save",    [=](const vector<string>&) { editor_gui->Top()->Save();             app->scheduler.Wakeup(0); });
  W->shell->Add("wrap",    [=](const vector<string>&) { editor_gui->Top()->ToggleShouldWrap(); app->scheduler.Wakeup(0); });
  W->shell->Add("gotodef", [=](const vector<string>&) { editor_gui->GotoDefinition();          app->scheduler.Wakeup(0); });
  W->shell->Add("open",    [=](const vector<string>&a){ editor_gui->Open(a.size() ? a[0]: ""); app->scheduler.Wakeup(0); });
}

}; // namespace LFL
using namespace LFL;

extern "C" void MyAppInit() {
  FLAGS_lfapp_video = FLAGS_lfapp_input = true;
  app->name = "LEdit";
  app->logfilename = StrCat(LFAppDownloadDir(), "ledit.txt");
  app->window_start_cb = MyWindowStart;
  app->window_init_cb = MyWindowInit;
  app->window_init_cb(screen);
}

extern "C" int MyAppMain(int argc, const char* const* argv) {
  if (app->Create(argc, argv, __FILE__)) return -1;
  screen->width = FLAGS_width;
  screen->height = FLAGS_height;

  if (app->Init()) return -1;
  app->scheduler.AddWaitForeverKeyboard();
  app->scheduler.AddWaitForeverMouse();

  vector<MenuItem> file_menu{ MenuItem{"o", "Open", "choose"}, MenuItem{"s", "Save", "save" } };
  app->AddNativeMenu("File", file_menu);
  app->AddNativeEditMenu();

  vector<MenuItem> view_menu{
    MenuItem{"=", "Zoom In", ""}, MenuItem{"-", "Zoom Out", ""},
    MenuItem{"w", "Wrap lines", "wrap"} };
  app->AddNativeMenu("View", view_menu);

  int optind = Singleton<FlagMap>::Get()->optind;
  if (optind >= argc) { fprintf(stderr, "Usage: %s [-flags] <file>\n", argv[0]); return -1; }

  app->StartNewWindow(screen);
  EditorGUI *editor_gui = screen->GetGUI<EditorGUI>(0);

  if (FLAGS_project.size()) {
    LocalFile ccf(FLAGS_project, "r");
    editor_gui->project = make_unique<IDE::Project>();
    editor_gui->project->LoadCMakeCompileCommandsFile(&ccf);
  }

  editor_gui->Open(argv[optind]);
  return app->Main();
}
