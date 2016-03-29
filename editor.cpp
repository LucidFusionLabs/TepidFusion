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
#include "core/app/ipc.h"

namespace LFL {
DEFINE_string(project, "",  "CMake compile_commands.json");
DEFINE_int   (width,   840, "Window width");
DEFINE_int   (height,  760, "Window height");
extern FlagOfType<bool> FLAGS_lfapp_network_;

struct MyAppState {
  unique_ptr<IDE::Project> project;
  SearchPaths search_paths;
  string build_bin;
  MyAppState() : search_paths(getenv("PATH")), build_bin(search_paths.Find("make")) {}
} *my_app;

struct EditorGUI : public GUI {
  static const int init_right_divider_w=224;
  Box top_center_pane, bottom_center_pane, left_pane, right_pane;
  Widget::Divider bottom_divider, right_divider;
  TabbedDialogInterface *right_pane_tabs=0;
  TabbedDialog<EditorDialog> source_tabs;
  TabbedDialog<DirectoryTreeDialog> dir_tabs;
  TabbedDialog<PropertyTreeDialog> options_tabs;
  DirectoryTreeDialog dir_tree;
  PropertyTreeDialog options_tree;
  unique_ptr<Terminal> build_terminal;
  vector<MenuItem> source_context_menu{ MenuItem{ "", "Go To Definition", "gotodef" } };
  vector<MenuItem> dir_context_menu{ MenuItem{ "b", "Build", "build" } };
  bool console_animating = 0;
  ProcessPipe build_process;

  EditorGUI() :
    bottom_divider(this, true, 0), right_divider(this, false, init_right_divider_w),
    source_tabs(this), dir_tabs(this), options_tabs(this),
    dir_tree    (screen->gd, app->fonts->Change(screen->default_font, 0, Color::black, Color::grey90)),
    options_tree(screen->gd, app->fonts->Change(screen->default_font, 0, Color::black, Color::grey90)) {
    Activate(); 

    dir_tree.deleted_cb = [&](){ right_divider.size=0; right_divider.changed=1; };
    if (my_app->project) dir_tree.title_text = string(my_app->project->dir.c_str(), DirNameLen(my_app->project->dir));
    if (my_app->project) dir_tree.view.Open(StrCat(dir_tree.title_text, LocalFile::Slash));
    dir_tree.view.InitContextMenu(bind([=](){ app->LaunchNativeContextMenu(dir_context_menu); }));
    dir_tree.view.selected_line_clicked_cb = [&](PropertyTree *t, PropertyTree::Id id) {
      if (auto n = &t->tree[id-1]) if (n->val.size() && n->val.back() != '/') Open(n->val);
    };
    dir_tabs.AddTab(&dir_tree);
    screen->gui.push_back(&dir_tree);

    options_tree.view.SetRoot(options_tree.view.AddNode(nullptr, "", PropertyTree::Children{
      options_tree.view.AddNode(nullptr, "Aaaa", PropertyTree::Children{
        options_tree.view.AddNode(nullptr, "A-sub1"), options_tree.view.AddNode(nullptr, "A-sub2"),
        options_tree.view.AddNode(nullptr, "A-sub3"), options_tree.view.AddNode(nullptr, "A-sub4")}),
      options_tree.view.AddNode(nullptr, "Bb", PropertyTree::Children{
        options_tree.view.AddNode(nullptr, "B-sub1"), options_tree.view.AddNode(nullptr, "B-sub2"),
        options_tree.view.AddNode(nullptr, "B-sub3"), options_tree.view.AddNode(nullptr, "B-sub4")}) }));
    options_tree.deleted_cb = [&](){ right_divider.size=0; right_divider.changed=1; };
    if (my_app->project) options_tree.title_text = my_app->project->dir;
    options_tabs.AddTab(&options_tree);
    screen->gui.push_back(&options_tree);

    build_terminal = make_unique<Terminal>(nullptr, screen->gd, screen->default_font);
    build_terminal->newline_mode = true;
  }

  Editor *Top() { return source_tabs.top ? &source_tabs.top->view : 0; }

  EditorDialog *Open(const string &fin) {
    static string prefix = "file://";
    string fn = PrefixMatch(fin, prefix) ? fin.substr(prefix.size()) : fin;
    INFO("Editor Open ", fn);
    EditorDialog *editor = new EditorDialog(screen->gd, screen->default_font, new LocalFile(fn, "r"), 1, 1); 
    editor->view.project = my_app->project.get();
    editor->view.line.SetAttrSource(&editor->view);
    editor->view.SetColors(Singleton<TextBox::SolarizedLightColors>::Get());
    editor->deleted_cb = [=](){ source_tabs.DelTab(editor); child_box.Clear(); delete editor; };
    editor->view.InitContextMenu(bind([=](){ app->LaunchNativeContextMenu(source_context_menu); }));
    source_tabs.AddTab(editor);
    child_box.Clear();
    return editor;
  }

  void Layout() {
    Reset();
    box = screen->Box();
    right_divider.LayoutDivideRight(box, &top_center_pane, &right_pane, -box.h);
    bottom_divider.LayoutDivideBottom(top_center_pane, &top_center_pane, &bottom_center_pane, -box.h);
    source_tabs.box = top_center_pane;
    source_tabs.tab_dim.y = screen->default_font->Height();
    source_tabs.Layout();
    if (1) right_pane_tabs = &dir_tabs;
    else   right_pane_tabs = &options_tabs;
    right_pane_tabs->box = right_pane;
    right_pane_tabs->tab_dim.y = screen->default_font->Height();
    right_pane_tabs->Layout();
    if (!child_box.Size()) child_box.PushNop();
  }

  int Frame(LFL::Window *W, unsigned clicks, int flag) {
    W->gd->DisableBlend();
    if (bottom_divider.changed || right_divider.changed) Layout();
    GUI::Draw();
    source_tabs.Draw();
    screen->gd->DrawMode(DrawMode::_2D);
    if (bottom_center_pane.h) build_terminal->Draw(bottom_center_pane, TextArea::DrawFlag::CheckResized);
    screen->gd->DrawMode(DrawMode::_2D);
    if (right_pane.w) right_pane_tabs->Draw();
    if (right_divider.changing) BoxOutline().Draw(Box::DelBorder(right_pane, Border(1,1,1,1)));
    if (bottom_divider.changing) BoxOutline().Draw(Box::DelBorder(bottom_center_pane, Border(1,1,1,1)));
    W->DrawDialogs();
    return 0;
  }

  void UpdateAnimating() { app->scheduler.SetAnimating(console_animating); }
  void OnConsoleAnimating() { console_animating = screen->console->animating; UpdateAnimating(); }
  void ShowProjectExplorer() { right_divider.size = init_right_divider_w; right_divider.changed=1; }
  void ShowBuildTerminal() { bottom_divider.size = screen->default_font->Height()*5; bottom_divider.changed=1; }

  void GotoDefinition() {
    FileNameAndOffset fo = Top() ? Top()->FindDefinition(screen->mouse) : FileNameAndOffset();
    if (fo.fn.empty()) return;
    INFO("Editor GotoDefinition ", fo.fn, " ", fo.offset, " ", fo.y, " ", fo.x);
    auto editor = Open(fo.fn);
    editor->view.CheckResized(Box(source_tabs.box.w, source_tabs.box.h-source_tabs.tab_dim.y));
    int lines = source_tabs.box.h / editor->view.font->Height();
    editor->view.SetVScroll(fo.y - lines/2);
    editor->view.cursor.i.y = lines/2 - 1;
    editor->view.UpdateCursorX(fo.x);
  }

  void Build() {
    if (bottom_divider.size < screen->default_font->Height()) ShowBuildTerminal();
    if (build_process.in) return;
    vector<const char*> argv{ my_app->build_bin.c_str(), nullptr };
    string build_dir = StrCat(my_app->project->dir, LocalFile::Slash, "term");
    CHECK(!build_process.Open(&argv[0], build_dir.c_str()));
    app->RunInNetworkThread([=](){ app->net->unix_client->AddConnectedSocket
      (fileno(build_process.in), new Connection::CallbackHandler
       ([=](Connection *c){ build_terminal->Write(c->rb.buf); c->ReadFlush(c->rb.size()); },
        [=](Connection *c){ build_process.Close(); })); });
  }
};

void MyWindowInit(Window *W) {
  W->width = FLAGS_width;
  W->height = FLAGS_height;
  W->caption = app->name;
  CHECK_EQ(0, W->NewGUI());
}

void MyWindowStart(Window *W) {
  EditorGUI *editor_gui = W->ReplaceGUI(0, make_unique<EditorGUI>());
  if (FLAGS_console) W->InitConsole(bind(&EditorGUI::OnConsoleAnimating, editor_gui));
  W->frame_cb = bind(&EditorGUI::Frame, editor_gui, _1, _2, _3);
  W->default_textbox = [=](){ return editor_gui->Top(); };

  BindMap *binds = W->AddInputController(make_unique<BindMap>());
  binds->Add('6', Key::Modifier::Cmd, Bind::CB(bind(&Shell::console, W->shell.get(), vector<string>())));
  binds->Add('o', Key::Modifier::Cmd, Bind::CB([=](){ W->shell->console(vector<string>(1, "choose")); }));
  binds->Add('s', Key::Modifier::Cmd, Bind::CB([=](){ W->shell->console(vector<string>(1, "save")); }));
  binds->Add('w', Key::Modifier::Cmd, Bind::CB([=](){ W->shell->console(vector<string>(1, "wrap")); }));

  W->shell = make_unique<Shell>(nullptr, nullptr, nullptr);
  W->shell->Add("choose",       [=](const vector<string>&) { app->LaunchNativeFileChooser(1,0,0,"open"); });
  W->shell->Add("save",         [=](const vector<string>&) { editor_gui->Top()->Save();             app->scheduler.Wakeup(0); });
  W->shell->Add("wrap",         [=](const vector<string>&) { editor_gui->Top()->ToggleShouldWrap(); app->scheduler.Wakeup(0); });
  W->shell->Add("gotodef",      [=](const vector<string>&) { editor_gui->GotoDefinition();          app->scheduler.Wakeup(0); });
  W->shell->Add("open",         [=](const vector<string>&a){ editor_gui->Open(a.size() ? a[0]: ""); app->scheduler.Wakeup(0); });
  W->shell->Add("build",        [=](const vector<string>&a){ editor_gui->Build();                   app->scheduler.Wakeup(0); });
  W->shell->Add("show_project", [=](const vector<string>&) { editor_gui->ShowProjectExplorer();     app->scheduler.Wakeup(0); });
  W->shell->Add("show_build",   [=](const vector<string>&) { editor_gui->ShowBuildTerminal();       app->scheduler.Wakeup(0); });
}

}; // namespace LFL
using namespace LFL;

extern "C" void MyAppCreate() {
  FLAGS_lfapp_video = FLAGS_lfapp_input = true;
  app = new Application();
  screen = new Window();
  my_app = new MyAppState();
  app->name = "LEdit";
  app->window_start_cb = MyWindowStart;
  app->window_init_cb = MyWindowInit;
  app->window_init_cb(screen);
  app->exit_cb = [](){ delete my_app; };
}

extern "C" int MyAppMain(int argc, const char* const* argv) {
  if (app->Create(argc, argv, __FILE__)) return -1;
  screen->width = FLAGS_width;
  screen->height = FLAGS_height;

  if (app->Init()) return -1;
  app->scheduler.AddWaitForeverKeyboard();
  app->scheduler.AddWaitForeverMouse();
  bool start_network_thread = !(FLAGS_lfapp_network_.override && !FLAGS_lfapp_network);
  if (start_network_thread) {
    app->net = make_unique<Network>();
    CHECK(app->CreateNetworkThread(false, true));
  }

  vector<MenuItem> file_menu{ MenuItem{"o", "Open", "choose"}, MenuItem{"s", "Save", "save" },
    MenuItem{"b", "Build", "build"} };
  app->AddNativeMenu("File", file_menu);
  app->AddNativeEditMenu();

  vector<MenuItem> view_menu{
    MenuItem{"=", "Zoom In", ""}, MenuItem{"-", "Zoom Out", ""}, MenuItem{"w", "Wrap lines", "wrap"},
    MenuItem{"", "Show Project Explorer", "show_project"},
    MenuItem{"", "Show Build Console", "show_build"} };
  app->AddNativeMenu("View", view_menu);

  int optind = Singleton<FlagMap>::Get()->optind;
  if (optind >= argc) { fprintf(stderr, "Usage: %s [-flags] <file>\n", argv[0]); return -1; }

  if (FLAGS_project.size()) {
    LocalFile ccf(FLAGS_project, "r");
    my_app->project = make_unique<IDE::Project>();
    my_app->project->LoadCMakeCompileCommandsFile(&ccf);
    my_app->project->dir = string(FLAGS_project.c_str(), DirNameLen(FLAGS_project));
    INFO("Project dir = ", my_app->project->dir);
    INFO("Found make = ", my_app->build_bin);
  }

  app->StartNewWindow(screen);
  screen->gd->ClearColor(Color::grey70);
  EditorGUI *editor_gui = screen->GetOwnGUI<EditorGUI>(0);

  editor_gui->Open(argv[optind]);
  return app->Main();
}
