/*******************************************************************************
 * TrackingConfigWidget
 *
 *   A side panel that lists every parameter in gem_tracking.conf as an
 *   editable field, grouped by detector block and by top-level cuts. It is
 *   two-way synchronized with the file:
 *     - "Apply" writes the edited values back (preserving comments/layout)
 *       and emits applied() so the owner can re-apply the config at runtime;
 *     - a QFileSystemWatcher refreshes the fields when the file is edited
 *       externally.
 *
 *   The panel is file-driven: it keeps the raw lines of the config and only
 *   rewrites the value portion of lines whose field actually changed, so the
 *   on-disk comments and formatting are left intact.
 ******************************************************************************/

#ifndef TRACKING_CONFIG_WIDGET_H
#define TRACKING_CONFIG_WIDGET_H

#include <QWidget>
#include <QString>

#include <string>
#include <vector>

class QLineEdit;
class QFileSystemWatcher;
class QVBoxLayout;

namespace tracking_dev {

class TrackingConfigWidget : public QWidget
{
    Q_OBJECT
public:
    explicit TrackingConfigWidget(const QString &confPath, QWidget *parent = nullptr);

signals:
    void applied();          // emitted after a successful write (or external reload)

private slots:
    void onApply();
    void onFileChanged(const QString &path);

private:
    struct ParsedItem {
        int         line_index;
        std::string block;   // "" for top-level entries
        std::string key;
        std::string value;   // display value (comment + trailing ';' stripped)
    };
    struct Entry {
        int         line_index;
        std::string block;
        std::string key;
        QLineEdit  *editor = nullptr;
    };

    bool readFileLines();                                  // load m_lines from disk
    void parseCurrent(std::vector<ParsedItem> &out) const; // scan m_lines
    void buildUi();                                        // create grouped editors
    void refreshEditors();                                 // re-read file -> update texts
    void writeFile();                                      // edited values -> file

    static std::string trim(const std::string &s);
    static std::string valuePart(const std::string &line); // value after '=' minus ;/#
    static std::string rebuildLine(const std::string &line, const std::string &val);

    QString                  m_path;
    std::vector<std::string> m_lines;
    std::vector<Entry>       m_entries;
    QFileSystemWatcher      *m_watcher = nullptr;
    QVBoxLayout             *m_host = nullptr;
};

};

#endif
