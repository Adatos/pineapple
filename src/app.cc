
#include <algorithm>
#include <fstream>
#include <iostream>
#include <string>
#include <sstream>
#include <vector>

#include <wx/wxprec.h>
#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif
#include <wx/menu.h>
#include <wx/process.h>
#include <wx/stream.h>
#include <wx/txtstrm.h>
#include <wx/utils.h>
#include <wx/webview.h>

#include "config.h"

namespace config {

/// Environment variable to pass server path
constexpr char server_script_env[] = "PINEAPPLE_SERVER";
/// Default server script if none given
constexpr char server_script_default[] = "venv/bin/python scripts/eridani-main serve";
/// Default size of main window on startup
constexpr int initial_width = 900;
constexpr int initial_height = 700;
/// Local baseurl
constexpr char base_url[] = "http://localhost:8888";
/// Initial url to open
constexpr char start_url[] = "/tree/demo/TestNotebook.ipynb";
/// Title prefix
constexpr char title[] = "Pineapple";
/// Special protocol prefix
constexpr char protocol_prefix[] = "$$$$";
/// Page that shows loading animation and loads actual page
constexpr char loading_html_filename[] = "html/loading.html";
/// What to put at beginning of window title
constexpr char title_prefix[] = "Pineapple - ";

} /// namespace config

std::string load_page;
bool load_page_loaded = false;

class MainFrame: public wxFrame
{
    enum {
        wxID_SAVE_HTML = 10000,
        wxID_NEW_COPY,
        wxID_INSERT,
        wxID_DELETE, wxID_UNDELETE,
        wxID_SPLIT, wxID_MERGE,
        wxID_MOVE_UP, wxID_MOVE_DOWN,
        wxID_RUN, wxID_RUN_ALL, wxID_RUN_ALL_ABOVE, wxID_RUN_ALL_BELOW,
        wxID_CELL_CODE, wxID_CELL_MARKDOWN, wxID_CELL_RAW,
    };
public:
    MainFrame(std::string url0, const wxString &title, const wxPoint &pos, const wxSize &size, bool indirect_load);
    static MainFrame *Spawn(std::string url, bool indirect_load=false);

    wxProcess *server;
    wxWebView *webview;
    std::string url;

    void OnMenuEvent(wxCommandEvent &event);
    void OnClose(wxCloseEvent &event);
    void OnSubprocessTerminate(wxProcessEvent &event);
    void OnError(wxWebViewEvent &event);
    void OnTitleChanged(wxWebViewEvent &event);
    void OnNewWindow(wxWebViewEvent &event);

private:
    wxDECLARE_EVENT_TABLE();
};

wxBEGIN_EVENT_TABLE(MainFrame, wxFrame)
    EVT_CLOSE(MainFrame::OnClose)
    EVT_WEBVIEW_ERROR(wxID_ANY, MainFrame::OnError)
    EVT_WEBVIEW_TITLE_CHANGED(wxID_ANY, MainFrame::OnTitleChanged)
    EVT_WEBVIEW_NEWWINDOW(wxID_ANY, MainFrame::OnNewWindow)
wxEND_EVENT_TABLE()

class MainApp: public wxApp
{
public:
    virtual bool OnInit();
    virtual int OnExit();
    void OnSubprocessTerminate(wxProcessEvent &event);

    wxProcess *server;
    MainFrame *frame;
private:
    wxDECLARE_EVENT_TABLE();

};

wxBEGIN_EVENT_TABLE(MainApp, wxApp)
    EVT_END_PROCESS(wxID_ANY, MainApp::OnSubprocessTerminate)
wxEND_EVENT_TABLE()

wxIMPLEMENT_APP(MainApp);

std::string replace_one(std::string &s, std::string mud, std::string gold)
{
    std::cout << s.length() << ", " << mud.length() << ", " << gold.length() << std::endl;
    return s.replace(s.find(mud), mud.length(), gold);
}

bool MainApp::OnInit()
{
    frame = MainFrame::Spawn(std::string(config::base_url) + std::string(config::start_url), true);

    wxString server_script;
    server = nullptr;
    if (!wxGetEnv(config::server_script_env, &server_script)) {
        server_script = config::server_script_default;
    }
    server = new wxProcess(frame);
    wxExecute(server_script,
        wxEXEC_ASYNC | wxEXEC_HIDE_CONSOLE | wxEXEC_MAKE_GROUP_LEADER,
        server);

    return true;
}

int MainApp::OnExit()
{
    std::cout << "APP OnExit()" << std::endl;
    if (server) {
        std::cout << "CLOSING SERVER" << std::endl;
        server->Kill(server->GetPid(), wxSIGTERM, wxKILL_CHILDREN);
    }
}

void MainApp::OnSubprocessTerminate(wxProcessEvent &event)
{
    std::cout << "SUBPROCESS TERMINATED" << std::endl;
}

MainFrame::MainFrame(std::string url0, const wxString &title,
    const wxPoint &pos, const wxSize &size, bool indirect_load)
    : wxFrame(nullptr, wxID_ANY, title, pos, size), url(url0)
{
    wxMenuBar *menubar = new wxMenuBar();
    wxMenu *menu_file = new wxMenu();
    menu_file->Append(wxID_NEW, "&New");
    menu_file->AppendSeparator();
    menu_file->Append(wxID_SAVE, "&Save");
    menu_file->AppendSeparator();
    menu_file->Append(wxID_NEW_COPY, "Make a copy");
    menu_file->Append(wxID_SAVE_HTML, "Download HTML");
    menu_file->AppendSeparator();
    menu_file->Append(wxID_EXIT, "&Quit");
    menubar->Append(menu_file, "&File");

    wxMenu *menu_edit = new wxMenu();
    menu_edit->Append(wxID_CUT, "Cut cell");
    menu_edit->Append(wxID_COPY, "Copy cell");
    menu_edit->Append(wxID_PASTE, "Paste cell below");
    menu_edit->AppendSeparator();
    menu_edit->Append(wxID_INSERT, "Insert cell below");
    menu_edit->AppendSeparator();
    menu_edit->Append(wxID_DELETE, "Delete cell");
    menu_edit->Append(wxID_UNDELETE, "Undelete cell");
    menu_edit->AppendSeparator();
    menu_edit->Append(wxID_SPLIT, "Split cell");
    menu_edit->Append(wxID_MERGE, "Merge cell below");
    menu_edit->AppendSeparator();
    menu_edit->Append(wxID_MOVE_UP, "Move cell up");
    menu_edit->Append(wxID_MOVE_DOWN, "Move cell down");
    menubar->Append(menu_edit, "&Edit");

    wxMenu *menu_cell = new wxMenu();
    menu_cell->Append(wxID_RUN, "Run");
    menu_cell->AppendSeparator();
    menu_cell->Append(wxID_RUN_ALL, "Run all");
    menu_cell->Append(wxID_RUN_ALL_ABOVE, "Run all above");
    menu_cell->Append(wxID_RUN_ALL_BELOW, "Run all below");

    wxMenu *menu_type = new wxMenu();
    menu_type->Append(wxID_CELL_CODE, "Code");
    menu_type->Append(wxID_CELL_MARKDOWN, "Markdown");
    menu_type->Append(wxID_CELL_RAW, "Raw");
    menu_cell->AppendSubMenu(menu_type, "Cell type");

    menubar->Append(menu_cell, "&Cell");

    wxMenu *menu_help = new wxMenu();
    menu_help->Append(wxID_ABOUT, "&About");
    menubar->Append(menu_help, "&Help");
    
    SetMenuBar(menubar);

    Bind(wxEVT_COMMAND_MENU_SELECTED, &MainFrame::OnMenuEvent, this);

    wxBoxSizer* frame_sizer = new wxBoxSizer(wxVERTICAL);

    webview = wxWebView::New(this, wxID_ANY);
    frame_sizer->Add(webview, 1, wxEXPAND, 10);

    if (indirect_load) {
        if (!load_page_loaded) {
            // Read loading page
            std::ifstream ifs(config::loading_html_filename);
            load_page = std::string(std::istreambuf_iterator<char>(ifs), std::istreambuf_iterator<char>());
            load_page_loaded = true;
        }
        // Do template replacement for url
        std::string contents{load_page};
        replace_one(contents, "{{url}}", url);
        std::cout << contents << std::endl;
        webview->SetPage(wxString(contents), "");
    } else {
        webview->LoadURL(url);
    }

    webview->Show();

    SetSizerAndFit(frame_sizer);
    SetSize(wxDefaultCoord, wxDefaultCoord, size.GetWidth(), size.GetHeight());
}

void MainFrame::OnError(wxWebViewEvent &event)
{
    std::cout << "ERROR" << std::endl;
    webview->LoadURL(url);
}

void jupyter_click_cell(wxWebView *wv, std::string id)
{
    std::string cmd = "Jupyter.menubar.element.find('#" + id + "').click();";
    wv->RunScript(cmd);
}

void MainFrame::OnMenuEvent(wxCommandEvent &event)
{
    std::cout << "MENU EVENT" << std::endl;
    switch (event.GetId()) {
        case wxID_CUT:
        {
            jupyter_click_cell(webview, "cut_cell");
            break;
        }
        case wxID_COPY:
        {
            jupyter_click_cell(webview, "copy_cell");
            break;
        }
        case wxID_PASTE:
        {
            jupyter_click_cell(webview, "paste_cell_below");
            break;
        }
        case wxID_INSERT:
        {
            jupyter_click_cell(webview, "insert_cell_below");
            break;
        }
        case wxID_DELETE:
        {
            jupyter_click_cell(webview, "delete_cell");
            break;
        }
        case wxID_UNDELETE:
        {
            jupyter_click_cell(webview, "undelete_cell");
            break;
        }
        case wxID_SPLIT:
        {
            jupyter_click_cell(webview, "split_cell");
            break;
        }
        case wxID_MERGE:
        {
            jupyter_click_cell(webview, "merge_cell_below");
            break;
        }
        case wxID_MOVE_UP:
        {
            jupyter_click_cell(webview, "move_cell_up");
            break;
        }
        case wxID_MOVE_DOWN:
        {
            jupyter_click_cell(webview, "move_cell_down");
            break;
        }
        case wxID_RUN:
        {
            jupyter_click_cell(webview, "run_cell");
            break;
        }
        case wxID_RUN_ALL:
        {
            jupyter_click_cell(webview, "run_all_cells");
            break;
        }
        case wxID_RUN_ALL_ABOVE:
        {
            jupyter_click_cell(webview, "run_all_cells_above");
            break;
        }
        case wxID_RUN_ALL_BELOW:
        {
            jupyter_click_cell(webview, "run_all_cells_below");
            break;
        }
        case wxID_CELL_CODE:
        {
            jupyter_click_cell(webview, "to_code");
            break;
        }
        case wxID_CELL_MARKDOWN:
        {
            jupyter_click_cell(webview, "to_markdown");
            break;
        }
        case wxID_CELL_RAW:
        {
            jupyter_click_cell(webview, "to_raw");
            break;
        }
        case wxID_SAVE:
        {
            jupyter_click_cell(webview, "save_checkpoint");
            break;
        }
        case wxID_SAVE_HTML:
        {
            jupyter_click_cell(webview, "download_markdown");
            break;
        }
        case wxID_NEW:
        {
            jupyter_click_cell(webview, "open_notebook");
            break;
        }
        case wxID_NEW_COPY:
        {
            jupyter_click_cell(webview, "copy_notebook");
            break;
        }
        case wxID_ABOUT:
        {
            std::stringstream ss;
            ss << config::version_full << "\n\n" << wxGetLibraryVersionInfo().ToString() << std::endl;            
            wxMessageBox(ss.str(), "About", wxOK | wxICON_INFORMATION);
            break;
        }
        case wxID_EXIT:
        {
            Close(true);
            break;
        }
        default:
        {
            std::cerr << "ERROR UNHANDLED MENU EVENT" << std::endl;
            break;
        }
    }
}

void MainFrame::OnClose(wxCloseEvent &event)
{
    std::cout << "CLOSE" << std::endl;
    std::cout << "CLOSING WEBVIEW" << std::endl;
    if (webview) {
        webview->Destroy();
    }
    std::cout << "DESTROY SELF" << std::endl;
    Destroy();
}

void MainFrame::OnTitleChanged(wxWebViewEvent &event)
{
    std::string title = event.GetString().ToStdString();
    std::cout << "TITLE CHANGED - " << title << std::endl;
    // Check if starts with $$$$
    std::string prefix = config::protocol_prefix;
    if (std::equal(prefix.begin(), prefix.end(), title.begin())) {
        // Prefix present
        std::string theUrl = title.substr(prefix.size());
        std::cout << "SPECIAL " << theUrl << std::endl;
        Spawn(config::base_url + theUrl);
        return;
    }
    // Otherwise actually change the title
    SetLabel(config::title_prefix + title);
}

MainFrame *MainFrame::Spawn(std::string url, bool indirect_load)
{
    MainFrame *child = new MainFrame(url, url,
        wxPoint(wxDefaultCoord, wxDefaultCoord),
        wxSize(config::initial_width, config::initial_height), indirect_load);
    child->Show();
    return child;
}

void MainFrame::OnNewWindow(wxWebViewEvent &event)
{
    wxString url(event.GetURL());
    std::cout << "NEW WINDOW " << url << std::endl;
    wxLaunchDefaultBrowser(url);
}
