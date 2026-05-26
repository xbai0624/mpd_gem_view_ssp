#ifndef ONLINE_ANALYSIS_INTERFACE_H
#define ONLINE_ANALYSIS_INTERFACE_H

////////////////////////////////////////////////////////////////////////////////
// OnlineAnalysisInterface
//
// A popup window that does what scripts/../analyze_run_multi_core does:
//   - split an EVIO run across several ./bin/replay processes,
//   - hadd-merge the per-split outputs into Rootfiles/<prefix>_run<RUN>.root
//     (and the matching _data_quality_check.root),
//   - then plot the data-quality histograms directly on this window
//     (instead of the script's PDF + evince step).
//
// All work happens on the GUI thread via QProcess (no extra worker threads);
// each replay's stdout streams into the log widget below.
////////////////////////////////////////////////////////////////////////////////

#include <QMainWindow>
#include <QStringList>
#include <QList>
#include <QString>

#include <vector>

class QLineEdit;
class QSpinBox;
class QPushButton;
class QPlainTextEdit;
class QComboBox;
class QProcess;
class QWidget;
class QGroupBox;
class QSplitter;
class HistoWidget;
class TH1;

class OnlineAnalysisInterface : public QMainWindow
{
    Q_OBJECT

public:
    OnlineAnalysisInterface(QWidget *parent = nullptr);
    ~OnlineAnalysisInterface();

private slots:
    void BrowsePedestal();
    void BrowseCommonMode();
    void BrowseRawDir();
    void BrowseOutDir();

    void StartAnalysis();

    void OnWorkerReadyRead();
    void OnWorkerFinished(int exitCode, int exitStatus);

    void OnMergeReadyRead();
    void OnMergeFinished(int exitCode, int exitStatus);

    void ShowPage(int idx);

private:
    struct AnalysisRequest {
        QString ped;
        QString cm;
        QString rawDir;
        QString outDir;
        QString pattern;
        QString resolvedPattern;
        QString prefix;
        QString replayBin;             // resolved at Prepare time (no class state)
        QStringList absFiles;
        int run = 0;
        int nSplit = 0;
        int nCore = 0;
        int nJob = 0;
        int nFound = 0;
    };

    void BuildUi();
    QWidget *BuildPathRow(QWidget *parent, QLineEdit *&edit,
                          QPushButton *&button,
                          const QString &placeholder);
    QGroupBox *BuildSettingsGroup(QWidget *parent);
    QGroupBox *BuildPlotsGroup(QWidget *parent);
    QGroupBox *BuildLogGroup(QWidget *parent);
    void ConfigureMainSplitter(QSplitter *mainSplitter, QSplitter *contentSplitter,
                               QWidget *plots, QWidget *settings, QWidget *log);
    void ApplyStyle();
    void WireSignals();

    void AppendLog(const QString &line);
    static QString ShellQuote(const QString &s);   // single-quote-escape for bash
    QString ResolveReplayBinary() const;     // returns "" if not found
    QString DetectRepoRoot() const;          // walks up from app dir
    bool PrepareAnalysisRequest(AnalysisRequest &request);
    void LogAnalysisPreamble(const AnalysisRequest &request);
    QString BuildWorkerCommand(const AnalysisRequest &request,
                               int core, int start, int end) const;
    void CreateWorkerProcesses(const AnalysisRequest &request);
    void StartWorkerProcesses();
    QString BuildMergeCommand() const;
    void StartMergeProcess();

    void Plot();
    QString DataQualityFilePath() const;
    bool LoadHistograms(const QString &path);
    void PopulatePageCombo();
    void SetControlsEnabled(bool on);
    void ClearHistos();

private:
    // ---- settings widgets ----
    QLineEdit  *m_edPed     = nullptr;
    QLineEdit  *m_edCm      = nullptr;
    QLineEdit  *m_edRawDir  = nullptr;
    QLineEdit  *m_edOutDir  = nullptr;   // output ROOT folder (default ./Rootfiles/)
    QLineEdit  *m_edPattern = nullptr;   // raw file glob, supports {RUN}
    QLineEdit  *m_edPrefix  = nullptr;   // output prefix, e.g. prad_gem_run
    QSpinBox   *m_spRun     = nullptr;
    QSpinBox   *m_spSplits  = nullptr;   // 0 = all available
    QSpinBox   *m_spCores   = nullptr;

    QPushButton *m_btnBrowsePed = nullptr;
    QPushButton *m_btnBrowseCm  = nullptr;
    QPushButton *m_btnBrowseDir = nullptr;
    QPushButton *m_btnBrowseOut = nullptr;
    QPushButton *m_btnStart     = nullptr;

    // ---- output area ----
    QPlainTextEdit *m_log       = nullptr;
    HistoWidget    *m_plotWidget = nullptr;
    QComboBox      *m_pageCombo    = nullptr;
    std::vector<TH1*> m_histos;        // owned; deleted in ClearHistos()

    // ---- runtime state ----
    QList<QProcess*> m_workers;
    int     m_activeWorkers = 0;
    int     m_currentRun    = 0;
    QString m_currentPrefix;
    QString m_currentOutDir;
    QString m_repoRoot;       // cached at construction; ./setup_env.sh + ./bin/replay live here
    int     m_workerFailures = 0;
    QProcess *m_mergeProc = nullptr;
};

#endif
