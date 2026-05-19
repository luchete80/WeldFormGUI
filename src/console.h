//From imgui
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <string>

#define PY_SSIZE_T_CLEAN
#ifdef BUILD_PYTHON
#include <Python.h>
#endif
#include "App.h"

//THIS IS FOR CHANGE STD OUTPUT , NOT PYTHON ONSOLAE
class CustomBuffer : public std::streambuf {
public:
    CustomBuffer(char* buffer, std::size_t size) {
        // Set the buffer's put area
        setp(buffer, buffer + size - 1); // Leave space for null terminator
    }

protected:
    // Override overflow to handle buffer overflow
    int_type overflow(int_type ch) override {
        if (ch != EOF) {
            if (pptr() < epptr()) {
                *pptr() = static_cast<char>(ch);
                pbump(1);
                return ch;
            } else {
                return EOF; // Buffer full
            }
        }
        return EOF;
    }

    // Override sync to add a null terminator
    int sync() override {
        if (pptr() != nullptr) {
            *pptr() = '\0'; // Null-terminate the string
        }
        return 0; // Success
    }
};

void initPython(){
#ifdef BUILD_PYTHON
PyObject* ioModule = PyImport_ImportModule("io");
#endif
}

struct ExampleAppConsole
{
    std::string           InputBuf;
    ImVector<char*>       Items;
    ImVector<const char*> Commands;
    ImVector<char*>       History;
    int                   HistoryPos;    // -1: new line, 0..History.Size-1 browsing history.
    ImGuiTextFilter       Filter;
    bool                  AutoScroll;
    bool                  ScrollToBottom;
    
    //ONLY FOR STD OUTPUT
    CustomBuffer*          customBuffer;
    char buffer[256];
    
    ExampleAppConsole()
    {
        //IMGUI_DEMO_MARKER("Examples/Console");
        ClearLog();
        InputBuf.reserve(256);
        HistoryPos = -1;

        // "CLASSIFY" is here to provide the test case where "C"+[tab] completes to "CL" and display multiple matches.
        Commands.push_back("HELP");
        Commands.push_back("HISTORY");
        Commands.push_back("CLEAR");
        Commands.push_back("CLASSIFY");
        AutoScroll = true;
        ScrollToBottom = false;
        //AddLog("Welcome to Dear ImGui!");
        
        //ONLY FOR STDOUT
        //cout << "Redirecting "<<endl;
        //customBuffer  = new CustomBuffer(buffer, 256);
        //std::streambuf* originalCoutBuffer = std::cout.rdbuf(customBuffer);
        //cout << "end"<<endl;
    }
    ~ExampleAppConsole()
    {
        ClearLog();
        for (int i = 0; i < History.Size; i++)
            free(History[i]);
    }

    // Portable helpers
    static int   Stricmp(const char* s1, const char* s2)         { int d; while ((d = toupper(*s2) - toupper(*s1)) == 0 && *s1) { s1++; s2++; } return d; }
    static int   Strnicmp(const char* s1, const char* s2, int n) { int d = 0; while (n > 0 && (d = toupper(*s2) - toupper(*s1)) == 0 && *s1) { s1++; s2++; n--; } return d; }
    static char* Strdup(const char* s)                           { IM_ASSERT(s); size_t len = strlen(s) + 1; void* buf = malloc(len); IM_ASSERT(buf); return (char*)memcpy(buf, (const void*)s, len); }
    static void  Strtrim(char* s)                                { char* str_end = s + strlen(s); while (str_end > s && str_end[-1] == ' ') str_end--; *str_end = 0; }
    bool HasPendingInput() const                                 { return !InputBuf.empty(); }

    void    ClearLog()
    {
        for (int i = 0; i < Items.Size; i++)
            free(Items[i]);
        Items.clear();
    }

    void    AddLog(const char* fmt, ...) IM_FMTARGS(2)
    {
        // FIXME-OPT
        char buf[1024];
        va_list args;
        va_start(args, fmt);
        vsnprintf(buf, IM_ARRAYSIZE(buf), fmt, args);
        buf[IM_ARRAYSIZE(buf)-1] = 0;
        va_end(args);
        Items.push_back(Strdup(buf));
    }

    void AddLogString(const std::string& text)
    {
        if (text.empty())
            return;
        AddLog("%s", text.c_str());
    }

    bool AddLogFile(const std::string& path)
    {
        std::ifstream file(path);
        if (!file.is_open())
            return false;

        std::string line;
        while (std::getline(file, line)) {
            AddLog("%s\n", line.c_str());
        }
        return true;
    }

    void    Draw(const char* title, bool* p_open)
    {
        ImGui::SetNextWindowSize(ImVec2(520, 600), ImGuiCond_FirstUseEver);
        if (!ImGui::Begin(title, p_open))
        {
            ImGui::End();
            return;
        }

        // As a specific feature guaranteed by the library, after calling Begin() the last Item represent the title bar.
        // So e.g. IsItemHovered() will return true when hovering the title bar.
        // Here we create a context menu only available from the title bar.
        if (ImGui::BeginPopupContextItem())
        {
            if (ImGui::MenuItem("Close Console"))
                *p_open = false;
            ImGui::EndPopup();
        }

        // Reserve enough left-over height for the multiline input area and buttons.
        const float input_height = ImGui::GetTextLineHeight() * 3.0f;
        const float footer_height_to_reserve =
            input_height +
            ImGui::GetStyle().ItemSpacing.y * 3.0f +
            ImGui::GetFrameHeightWithSpacing() * 2.0f;
        ImGui::BeginChild("ScrollingRegion", ImVec2(0, -footer_height_to_reserve), false, ImGuiWindowFlags_HorizontalScrollbar);
        if (ImGui::BeginPopupContextWindow())
        {
            if (ImGui::Selectable("Clear")) ClearLog();
            ImGui::EndPopup();
        }

        // Display every line as a separate entry so we can change their color or add custom widgets.
        // If you only want raw text you can use ImGui::TextUnformatted(log.begin(), log.end());
        // NB- if you have thousands of entries this approach may be too inefficient and may require user-side clipping
        // to only process visible items. The clipper will automatically measure the height of your first item and then
        // "seek" to display only items in the visible area.
        // To use the clipper we can replace your standard loop:
        //      for (int i = 0; i < Items.Size; i++)
        //   With:
        //      ImGuiListClipper clipper;
        //      clipper.Begin(Items.Size);
        //      while (clipper.Step())
        //         for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++)
        // - That your items are evenly spaced (same height)
        // - That you have cheap random access to your elements (you can access them given their index,
        //   without processing all the ones before)
        // You cannot this code as-is if a filter is active because it breaks the 'cheap random-access' property.
        // We would need random-access on the post-filtered list.
        // A typical application wanting coarse clipping and filtering may want to pre-compute an array of indices
        // or offsets of items that passed the filtering test, recomputing this array when user changes the filter,
        // and appending newly elements as they are inserted. This is left as a task to the user until we can manage
        // to improve this example code!
        // If your items are of variable height:
        // - Split them into same height items would be simpler and facilitate random-seeking into your list.
        // - Consider using manual call to IsRectVisible() and skipping extraneous decoration from your items.
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 1)); // Tighten spacing
        bool copy_to_clipboard = false;
        if (copy_to_clipboard)
            ImGui::LogToClipboard();
        for (int i = 0; i < Items.Size; i++)
        {
            const char* item = Items[i];
            if (!Filter.PassFilter(item))
                continue;

            // Normally you would store more information in your item than just a string.
            // (e.g. make Items[] an array of structure, store color/type etc.)
            ImVec4 color;
            bool has_color = false;
            if (strstr(item, "[error]"))          { color = ImVec4(1.0f, 0.4f, 0.4f, 1.0f); has_color = true; }
            else if (strncmp(item, "# ", 2) == 0) { color = ImVec4(1.0f, 0.8f, 0.6f, 1.0f); has_color = true; }
            if (has_color)
                ImGui::PushStyleColor(ImGuiCol_Text, color);
            ImGui::TextUnformatted(item);
            if (has_color)
                ImGui::PopStyleColor();
        }
        if (copy_to_clipboard)
            ImGui::LogFinish();

        if (ScrollToBottom || (AutoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()))
            ImGui::SetScrollHereY(1.0f);
        ScrollToBottom = false;

        ImGui::PopStyleVar();
        ImGui::EndChild();
        ImGui::Separator();

        // Command-line
        bool reclaim_focus = false;
        ImGuiInputTextFlags input_text_flags =
            ImGuiInputTextFlags_CallbackCompletion |
            ImGuiInputTextFlags_CallbackHistory |
            ImGuiInputTextFlags_CallbackResize;
        ImGui::TextUnformatted("Input");
        ImGui::InputTextMultiline(
            "##Input",
            InputBuf.data(),
            InputBuf.capacity() + 1,
            ImVec2(-1.0f, input_height),
            input_text_flags,
            &TextEditCallbackStub,
            (void*)this);

        InputBuf.resize(std::strlen(InputBuf.c_str()));
        char* s = InputBuf.data();
        Strtrim(s);

        ImGuiIO& io = ImGui::GetIO();
        const bool run_shortcut =
            ImGui::IsItemFocused() &&
            io.KeyCtrl &&
            !io.KeyAlt &&
            !io.KeySuper &&
            ImGui::IsKeyPressed(ImGuiKey_Enter, false);
        const bool run_button = ImGui::Button("Run");
        ImGui::SameLine();
        if (ImGui::Button("Clear Input")) {
            InputBuf.clear();
            reclaim_focus = true;
        }
        ImGui::SameLine();
        copy_to_clipboard = ImGui::Button("Copy Log");
        ImGui::SameLine();
        if (ImGui::Button("Clear Log")) {
            ClearLog();
        }
        ImGui::SameLine();
        ImGui::TextUnformatted("Ctrl+Enter to run");

        if ((run_shortcut || run_button) && s[0]) {
            InputBuf.resize(std::strlen(InputBuf.c_str()));
            ExecCommand(InputBuf.c_str());
            InputBuf.clear();
            reclaim_focus = true;
        }

        // Auto-focus on window apparition
        ImGui::SetItemDefaultFocus();
        if (reclaim_focus)
            ImGui::SetKeyboardFocusHere(-1); // Auto focus previous widget

        ImGui::End();
    }

    void    ExecCommand(const char* command_line)
    {
        AddLog("# %s\n", command_line);

        // Insert into history. First find match and delete it so it can be pushed to the back.
        // This isn't trying to be smart or optimal.
        HistoryPos = -1;
        //system(command_line);
    ///////////////////// BEFORE /////////////////////////////////////////////////
    
    #ifdef BUILD_PYTHON
    PyObject* ioModule = PyImport_ImportModule("io");
    if (!ioModule) {
        std::cerr << "Failed to import io module!" << std::endl;
        cout << "ERROR!!"<<endl;
        return;
    } 
    PyObject* stringIOClass = PyObject_GetAttrString(ioModule, "StringIO");
    Py_DECREF(ioModule);
    if (!stringIOClass) {
        std::cerr << "Failed to get StringIO class!" << std::endl;
        cout << "ERROR!!"<<endl;
        return;
    }    
    
    // Create a StringIO object
    PyObject* stringIO = PyObject_CallObject(stringIOClass, nullptr);
    Py_DECREF(stringIOClass);
    if (!stringIO) {
        cout << "ERROR!!"<<endl;
        std::cerr << "Failed to create StringIO object!" << std::endl;
        return;
    }

    // Redirect sys.stdout and sys.stderr to the StringIO object
    PyObject* sysModule = PyImport_ImportModule("sys");
    if (!sysModule) {
        std::cerr << "Failed to import sys module!" << std::endl;
        Py_DECREF(stringIO);
        cout << "ERROR!!"<<endl;
        return;
    }
    PyObject* originalStdout = PyObject_GetAttrString(sysModule, "stdout");
    PyObject* originalStderr = PyObject_GetAttrString(sysModule, "stderr");
    if (!originalStdout || !originalStderr) {
        std::cerr << "Failed to access sys.stdout/sys.stderr!" << std::endl;
        Py_XDECREF(originalStdout);
        Py_XDECREF(originalStderr);
        Py_DECREF(sysModule);
        Py_DECREF(stringIO);
        cout << "ERROR!!"<<endl;
        return;
    }
    PyObject_SetAttrString(sysModule, "stdout", stringIO);
    PyObject_SetAttrString(sysModule, "stderr", stringIO);
    
    ////////////////////////
    PyRun_SimpleString(command_line);
    getApp().Update(); //
////////////////////////////////////////////////////////////////////////
 /// AFTER
/////////////////////////////////////////////////
    PyObject* getValueMethod = PyObject_GetAttrString(stringIO, "getvalue");
    PyObject* output = getValueMethod ? PyObject_CallObject(getValueMethod, nullptr) : nullptr;
    Py_XDECREF(getValueMethod);

    if (PyObject_SetAttrString(sysModule, "stdout", originalStdout) != 0) {
        std::cerr << "Failed to restore sys.stdout!" << std::endl;
    }
    if (PyObject_SetAttrString(sysModule, "stderr", originalStderr) != 0) {
        std::cerr << "Failed to restore sys.stderr!" << std::endl;
    }

    std::string capturedOutput;
    if (output) {
        const char* outputCStr = PyUnicode_AsUTF8(output);
        if (outputCStr) {
            capturedOutput = outputCStr;
        }
        Py_DECREF(output);
    } else {
        std::cerr << "Failed to retrieve output from StringIO!" << std::endl;
    }

    Py_DECREF(originalStdout);
    Py_DECREF(originalStderr);
    Py_DECREF(sysModule);
    Py_DECREF(stringIO);

    if (!capturedOutput.empty()) {
        AddLog("%s", capturedOutput.c_str());
        std::cout << capturedOutput;
        std::cout.flush();
    }
 //////////////////////////////////
                       
        for (int i = History.Size - 1; i >= 0; i--)
            if (Stricmp(History[i], command_line) == 0)
            {
                free(History[i]);
                History.erase(History.begin() + i);
                break;
            }
        History.push_back(Strdup(command_line));

        // Process command
        if (Stricmp(command_line, "CLEAR") == 0)
        {
            ClearLog();
        }
        else if (Stricmp(command_line, "HELP") == 0)
        {
            AddLog("Commands:");
            for (int i = 0; i < Commands.Size; i++)
                AddLog("- %s", Commands[i]);
        }
        else if (Stricmp(command_line, "HISTORY") == 0)
        {
            int first = History.Size - 10;
            for (int i = first > 0 ? first : 0; i < History.Size; i++)
                AddLog("%3d: %s\n", i, History[i]);
        }
        else
        {
            //AddLog("Unknown command: '%s'\n", command_line);
        }

        // On command input, we scroll to bottom even if AutoScroll==false
        ScrollToBottom = true;
        
        #endif
    }

    // In C++11 you'd be better off using lambdas for this sort of forwarding callbacks
    static int TextEditCallbackStub(ImGuiInputTextCallbackData* data)
    {
        ExampleAppConsole* console = (ExampleAppConsole*)data->UserData;
        return console->TextEditCallback(data);
    }

    int     TextEditCallback(ImGuiInputTextCallbackData* data)
    {
        if (data->EventFlag == ImGuiInputTextFlags_CallbackResize)
        {
            InputBuf.resize(static_cast<std::size_t>(data->BufTextLen));
            data->Buf = InputBuf.data();
            return 0;
        }

        #ifdef BUILD_PYTHON
        //AddLog("cursor: %d, selection: %d-%d", data->CursorPos, data->SelectionStart, data->SelectionEnd);
        switch (data->EventFlag)
        {
        case ImGuiInputTextFlags_CallbackCompletion:
            {
                // Example of TEXT COMPLETION

                // Locate beginning of current word
                const char* word_end = data->Buf + data->CursorPos;
                const char* word_start = word_end;
                while (word_start > data->Buf)
                {
                    const char c = word_start[-1];
                    if (c == ' ' || c == '\t' || c == ',' || c == ';')
                        break;
                    word_start--;
                }

                // Build a list of candidates
                ImVector<const char*> candidates;
                for (int i = 0; i < Commands.Size; i++)
                    if (Strnicmp(Commands[i], word_start, (int)(word_end - word_start)) == 0)
                        candidates.push_back(Commands[i]);

                if (candidates.Size == 0)
                {
                    // No match
                    AddLog("No match for \"%.*s\"!\n", (int)(word_end - word_start), word_start);
                }
                else if (candidates.Size == 1)
                {
                    // Single match. Delete the beginning of the word and replace it entirely so we've got nice casing.
                    data->DeleteChars((int)(word_start - data->Buf), (int)(word_end - word_start));
                    data->InsertChars(data->CursorPos, candidates[0]);
                    data->InsertChars(data->CursorPos, " ");
                }
                else
                {
                    // Multiple matches. Complete as much as we can..
                    // So inputing "C"+Tab will complete to "CL" then display "CLEAR" and "CLASSIFY" as matches.
                    int match_len = (int)(word_end - word_start);
                    for (;;)
                    {
                        int c = 0;
                        bool all_candidates_matches = true;
                        for (int i = 0; i < candidates.Size && all_candidates_matches; i++)
                            if (i == 0)
                                c = toupper(candidates[i][match_len]);
                            else if (c == 0 || c != toupper(candidates[i][match_len]))
                                all_candidates_matches = false;
                        if (!all_candidates_matches)
                            break;
                        match_len++;
                    }

                    if (match_len > 0)
                    {
                        data->DeleteChars((int)(word_start - data->Buf), (int)(word_end - word_start));
                        data->InsertChars(data->CursorPos, candidates[0], candidates[0] + match_len);
                    }

                    // List matches
                    AddLog("Possible matches:\n");
                    for (int i = 0; i < candidates.Size; i++)
                        AddLog("- %s\n", candidates[i]);
                }

                break;
            }
        case ImGuiInputTextFlags_CallbackHistory:
            {
                // Example of HISTORY
                const int prev_history_pos = HistoryPos;
                if (data->EventKey == ImGuiKey_UpArrow)
                {
                    if (HistoryPos == -1)
                        HistoryPos = History.Size - 1;
                    else if (HistoryPos > 0)
                        HistoryPos--;
                }
                else if (data->EventKey == ImGuiKey_DownArrow)
                {
                    if (HistoryPos != -1)
                        if (++HistoryPos >= History.Size)
                            HistoryPos = -1;
                }

                // A better implementation would preserve the data on the current input line along with cursor position.
                if (prev_history_pos != HistoryPos)
                {
                    const char* history_str = (HistoryPos >= 0) ? History[HistoryPos] : "";
                    data->DeleteChars(0, data->BufTextLen);
                    data->InsertChars(0, history_str);
                }
            }
        }
        #endif
        return 0;
    }
};
