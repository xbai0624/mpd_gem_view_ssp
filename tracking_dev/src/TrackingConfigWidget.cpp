#include "TrackingConfigWidget.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QToolBox>
#include <QTabWidget>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QFileSystemWatcher>
#include <QFileInfo>
#include <QString>

#include <fstream>
#include <sstream>
#include <unordered_map>
#include <iostream>

namespace tracking_dev {

TrackingConfigWidget::TrackingConfigWidget(const QString &confPath, QWidget *parent)
    : QWidget(parent), m_path(confPath)
{
    buildUi();

    // watch the file for external edits
    m_watcher = new QFileSystemWatcher(this);
    if(!m_path.isEmpty())
        m_watcher->addPath(m_path);
    connect(m_watcher, &QFileSystemWatcher::fileChanged,
            this, &TrackingConfigWidget::onFileChanged);
}

////////////////////////////////////////////////////////////////////////////////
// string helpers

std::string TrackingConfigWidget::trim(const std::string &s)
{
    const char *ws = " \t\r\n";
    size_t a = s.find_first_not_of(ws);
    if(a == std::string::npos) return "";
    size_t b = s.find_last_not_of(ws);
    return s.substr(a, b - a + 1);
}

// value text after '=', with trailing comment and ';' removed
std::string TrackingConfigWidget::valuePart(const std::string &line)
{
    size_t eq = line.find('=');
    if(eq == std::string::npos) return "";
    size_t hash = line.find('#', eq);
    std::string v = line.substr(eq + 1,
            (hash == std::string::npos ? line.size() : hash) - (eq + 1));
    v = trim(v);
    if(!v.empty() && v.back() == ';') {
        v.pop_back();
        v = trim(v);
    }
    return v;
}

// rebuild a line keeping its key/'=', re-adding ';' if the original had one,
// and re-appending the trailing comment.
std::string TrackingConfigWidget::rebuildLine(const std::string &line,
                                              const std::string &val)
{
    size_t eq = line.find('=');
    if(eq == std::string::npos) return line;
    size_t hash = line.find('#', eq);
    std::string region = line.substr(eq + 1,
            (hash == std::string::npos ? line.size() : hash) - (eq + 1));
    bool semi = region.find(';') != std::string::npos;

    std::string head = line.substr(0, eq + 1);          // "  key ="
    std::string tail = (hash == std::string::npos) ? "" : line.substr(hash);

    std::string res = head + " " + val;
    if(semi) res += ";";
    if(!tail.empty()) res += "  " + tail;
    return res;
}

////////////////////////////////////////////////////////////////////////////////
// file IO + parsing

bool TrackingConfigWidget::readFileLines()
{
    m_lines.clear();
    std::ifstream f(m_path.toStdString());
    if(!f.is_open()) {
        std::cout << "TrackingConfigWidget: cannot open " << m_path.toStdString() << std::endl;
        return false;
    }
    std::string line;
    while(std::getline(f, line))
        m_lines.push_back(line);
    return true;
}

void TrackingConfigWidget::parseCurrent(std::vector<ParsedItem> &out) const
{
    out.clear();
    std::string current_block;
    for(int idx = 0; idx < (int)m_lines.size(); ++idx)
    {
        const std::string &raw = m_lines[idx];
        std::string t = trim(raw);
        if(t.empty() || t[0] == '#') continue;

        if(t.back() == '{') {                 // "name = {"
            size_t eq = t.find('=');
            current_block = (eq == std::string::npos) ? "" : trim(t.substr(0, eq));
            continue;
        }
        if(t[0] == '}') { current_block.clear(); continue; }

        size_t eq = raw.find('=');
        if(eq == std::string::npos) continue;

        ParsedItem it;
        it.line_index = idx;
        it.block = current_block;
        it.key   = trim(raw.substr(0, eq));
        it.value = valuePart(raw);
        out.push_back(it);
    }
}

////////////////////////////////////////////////////////////////////////////////
// UI

void TrackingConfigWidget::buildUi()
{
    // exact stylesheet block from gui/src/Viewer.cpp -- note it does NOT
    // style QWidget/QPushButton/QLineEdit, so buttons & inputs keep the
    // native look identical to the main GUI.
    setStyleSheet(R"(
            QMainWindow {
                 background: #FFFFEF;
            }
            QGroupBox {
                 border: 1px solid #D0D3D8;
                 border-radius: 6px;
                 margin-top: 12px;
                 background: #FFFFFF;
            }
            QGroupBox::title {
                 subcontrol-origin: margin;
                 left: 10px;
            }
            QToolBox::tab {
                 background: #E8EAED;
                 border-radius: 4px;
                 padding: 0px, 0px;
                 margin: 2px;
            }
            QPlainTextEdit {
                background: #FFFFFF;
                color: #000000;
                border-radius: 0px;
            }
            )");

    m_host = new QVBoxLayout(this);
    m_host->setContentsMargins(6, 6, 6, 6);
    m_host->setSpacing(6);

    QLabel *title = new QLabel(tr("Tracking Configuration"), this);
    title->setStyleSheet("font-weight: bold; font-size: 14px; padding: 2px 0;");
    m_host->addWidget(title);

    // show only the file name (single line, muted); full path on hover.
    QLabel *path = new QLabel(QFileInfo(m_path).fileName(), this);
    path->setStyleSheet("color: #888; font-style: italic; padding-bottom: 4px;");
    path->setToolTip(m_path);
    m_host->addWidget(path);

    if(!readFileLines()) {
        m_host->addWidget(new QLabel(tr("(could not read config file)"), this));
        m_host->addStretch(1);
        return;
    }

    std::vector<ParsedItem> items;
    parseCurrent(items);

    // Two-level layout:
    //   QToolBox (vertical accordion)
    //     |- "Detectors"  -> QTabWidget with one tab per gemN block
    //     |- "Parameters" -> form of the top-level tracking/cluster cuts
    QToolBox *toolbox = new QToolBox(this);
    m_host->addWidget(toolbox, 1);

    QTabWidget *detTabs = new QTabWidget(toolbox);
    toolbox->addItem(detTabs, tr("Detectors"));

    QWidget *paramPage = new QWidget(toolbox);
    QFormLayout *paramForm = new QFormLayout(paramPage);
    toolbox->addItem(paramPage, tr("Parameters"));

    std::unordered_map<std::string, QFormLayout*> detForms;
    auto formFor = [&](const std::string &block) -> QFormLayout*
    {
        if(block.empty()) return paramForm;            // top-level cuts

        auto it = detForms.find(block);
        if(it != detForms.end()) return it->second;

        QWidget *page = new QWidget(detTabs);          // one tab per detector
        QFormLayout *form = new QFormLayout(page);
        detTabs->addTab(page, QString::fromStdString(block));
        detForms[block] = form;
        return form;
    };

    for(const ParsedItem &it : items)
    {
        QLineEdit *ed = new QLineEdit(QString::fromStdString(it.value), this);
        formFor(it.block)->addRow(QString::fromStdString(it.key), ed);

        Entry e;
        e.line_index = it.line_index;
        e.block = it.block;
        e.key = it.key;
        e.editor = ed;
        m_entries.push_back(e);
    }

    // apply / reload buttons
    QHBoxLayout *btnRow = new QHBoxLayout();
    QPushButton *applyBtn = new QPushButton(tr("Apply"), this);
    QPushButton *reloadBtn = new QPushButton(tr("Reload from file"), this);
    btnRow->addWidget(applyBtn);
    btnRow->addWidget(reloadBtn);
    btnRow->addStretch(1);
    m_host->addLayout(btnRow);

    connect(applyBtn,  &QPushButton::clicked, this, &TrackingConfigWidget::onApply);
    connect(reloadBtn, &QPushButton::clicked, this, [this]{ refreshEditors(); });
}

void TrackingConfigWidget::refreshEditors()
{
    if(!readFileLines()) return;

    std::vector<ParsedItem> items;
    parseCurrent(items);

    // update each existing editor by matching (block, key); also refresh its
    // line_index in case lines shifted.
    for(const ParsedItem &it : items) {
        for(Entry &e : m_entries) {
            if(e.block == it.block && e.key == it.key) {
                e.line_index = it.line_index;
                if(e.editor->text().toStdString() != it.value)
                    e.editor->setText(QString::fromStdString(it.value));
                break;
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////
// write-back

void TrackingConfigWidget::writeFile()
{
    // only rewrite lines whose value actually changed -> minimal diff
    for(const Entry &e : m_entries) {
        if(e.line_index < 0 || e.line_index >= (int)m_lines.size()) continue;
        const std::string newVal = trim(e.editor->text().toStdString());
        const std::string curVal = valuePart(m_lines[e.line_index]);
        if(newVal == curVal) continue;
        m_lines[e.line_index] = rebuildLine(m_lines[e.line_index], newVal);
    }

    std::ofstream out(m_path.toStdString(), std::ios::trunc);
    if(!out.is_open()) {
        std::cout << "TrackingConfigWidget: cannot write " << m_path.toStdString() << std::endl;
        return;
    }
    for(const std::string &l : m_lines)
        out << l << "\n";
}

void TrackingConfigWidget::onApply()
{
    // re-read first so we rewrite on top of the latest on-disk content
    readFileLines();

    // Stop watching while WE write, so our own write doesn't bounce back as
    // an external change (fileChanged is delivered asynchronously, so a flag
    // alone wouldn't reliably cover it). Re-add the watch afterwards.
    if(m_watcher && !m_path.isEmpty())
        m_watcher->removePath(m_path);

    writeFile();

    if(m_watcher && !m_path.isEmpty())
        m_watcher->addPath(m_path);

    emit applied();
}

void TrackingConfigWidget::onFileChanged(const QString &path)
{
    refreshEditors();

    // editors that do a replace-then-rename may invalidate the watch
    if(m_watcher && !m_path.isEmpty() && !m_watcher->files().contains(path))
        m_watcher->addPath(path);

    emit applied();
}

};
