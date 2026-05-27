#include "OnlineAnalysisInterface.h"

#include "ConfigObject.h"  // same parser Viewer uses for config/gem.conf
#include "HistoWidget.h"

#include <QWidget>
#include <QGroupBox>
#include <QFormLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QSpinBox>
#include <QPushButton>
#include <QPlainTextEdit>
#include <QComboBox>
#include <QLabel>
#include <QListWidget>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QProcess>
#include <QSplitter>
#include <QMessageBox>
#include <QTextCursor>
#include <QScrollBar>
#include <QCoreApplication>
#include <QTextStream>

#include <vector>
#include <cstdio>

#include <TFile.h>
#include <TKey.h>
#include <TROOT.h>
#include <TH1.h>
#include <TH2.h>
#include <TClass.h>

#include <QtAlgorithms>
#include <QFontMetrics>

#include <cmath>

////////////////////////////////////////////////////////////////////////////////
// ctor / dtor

OnlineAnalysisInterface::OnlineAnalysisInterface(QWidget *parent)
    : QMainWindow(parent)
{
    resize(1100, 700);
    setWindowTitle(tr("Online Analysis"));
    // detect repo root ONCE: applicationDirPath's grandparent (or higher) is
    // where setup_env.sh + bin/replay live, regardless of where the user
    // launched the GUI from. Fall back to current CWD if not found.
    m_repoRoot = DetectRepoRoot();
    BuildUi();
}

OnlineAnalysisInterface::~OnlineAnalysisInterface()
{
    // make sure no background processes outlive us
    for(auto *p : m_workers) {
        if(p && p->state() != QProcess::NotRunning) {
            p->kill();
            p->waitForFinished(1000);
        }
    }
    if(m_mergeProc && m_mergeProc->state() != QProcess::NotRunning) {
        m_mergeProc->kill();
        m_mergeProc->waitForFinished(1000);
    }
    // release any histograms we own
    for(TH1 *h : m_histos) delete h;
    m_histos.clear();
}

////////////////////////////////////////////////////////////////////////////////
// UI construction

void OnlineAnalysisInterface::BuildUi()
{
    QWidget *central = new QWidget(this);
    QVBoxLayout *outer = new QVBoxLayout(central);

    QGroupBox *gbPlots = BuildPlotsGroup(central);
    QGroupBox *gbSet = BuildSettingsGroup(central);
    QGroupBox *gbLog = BuildLogGroup(central);

    QSplitter *mainSplitter = new QSplitter(Qt::Vertical, central);
    outer->addWidget(mainSplitter, 1);
    QSplitter *splitter = new QSplitter(Qt::Horizontal, central);

    ConfigureMainSplitter(mainSplitter, splitter, gbPlots, gbSet, gbLog);
    setCentralWidget(central);
    ApplyStyle();
    WireSignals();
}

QWidget *OnlineAnalysisInterface::BuildPathRow(QWidget *parent, QLineEdit *&edit,
                                               QPushButton *&button,
                                               const QString &placeholder)
{
    QWidget *row = new QWidget(parent);
    QHBoxLayout *layout = new QHBoxLayout(row);
    layout->setContentsMargins(0, 0, 0, 0);

    edit = new QLineEdit(row);
    edit->setPlaceholderText(placeholder);
    button = new QPushButton(tr("Browse..."), row);

    layout->addWidget(edit, 1);
    layout->addWidget(button);
    return row;
}

QGroupBox *OnlineAnalysisInterface::BuildSettingsGroup(QWidget *parent)
{
    QGroupBox *gbSet = new QGroupBox(tr("Settings"), parent);
    // Cap the settings pane width to ~40 average characters of the actual
    // UI font; survives high-DPI scaling better than a hardcoded 320 px.
    const int em = gbSet->fontMetrics().averageCharWidth();
    gbSet->setMaximumWidth(qMax(320, em * 40));
    QFormLayout *form = new QFormLayout(gbSet);

    QWidget *rowPed = BuildPathRow(gbSet, m_edPed, m_btnBrowsePed,
                                   "database/gem_ped_*.dat");
    QWidget *rowCm = BuildPathRow(gbSet, m_edCm, m_btnBrowseCm,
                                  "database/CommonModeRange_*.txt");
    QWidget *rowDir = BuildPathRow(gbSet, m_edRawDir, m_btnBrowseDir,
                                   "../evio_data/prad2/");
    QWidget *rowOut = BuildPathRow(gbSet, m_edOutDir, m_btnBrowseOut,
                                   "./Rootfiles/");
    m_edOutDir->setText("./Rootfiles/");

    ConfigObject cfg;
    if(cfg.ReadConfigFile("config/gem.conf")) {
        m_edPed->setText(QString::fromStdString(
                         cfg.Value<std::string>("GEM Pedestal")));
        m_edCm->setText(QString::fromStdString(
                        cfg.Value<std::string>("GEM Common Mode")));
    }

    m_edPattern = new QLineEdit("prad_0{RUN}.evio.*", gbSet);
    m_edPrefix = new QLineEdit("prad_gem_run", gbSet);

    m_spRun = new QSpinBox(gbSet);
    m_spRun->setRange(0, 9999999);
    m_spRun->setValue(0);

    m_spSplits = new QSpinBox(gbSet);
    m_spSplits->setRange(0, 9999);
    m_spSplits->setValue(0);
    m_spSplits->setSpecialValueText(tr("all"));

    m_spCores = new QSpinBox(gbSet);
    m_spCores->setRange(1, 256);
    m_spCores->setValue(1);

    form->addRow(tr("Pedestal file:"), rowPed);
    form->addRow(tr("Common-mode file:"), rowCm);
    form->addRow(tr("Raw data folder:"), rowDir);
    form->addRow(tr("Output folder:"), rowOut);
    form->addRow(tr("Raw file pattern:"), m_edPattern);
    form->addRow(tr("Output prefix:"), m_edPrefix);
    form->addRow(tr("Run number:"), m_spRun);
    form->addRow(tr("Splits to replay:"), m_spSplits);
    form->addRow(tr("CPU cores:"), m_spCores);

    m_btnStart = new QPushButton(tr("Start Analysis"), gbSet);
    form->addRow(QString(), m_btnStart);
    return gbSet;
}

QGroupBox *OnlineAnalysisInterface::BuildPlotsGroup(QWidget *parent)
{
    QGroupBox *gbPlots = new QGroupBox(tr("Analysis Result Plots"), parent);
    QVBoxLayout *vplots = new QVBoxLayout(gbPlots);

    QWidget *topRow = new QWidget(gbPlots);
    QHBoxLayout *topRowLayout = new QHBoxLayout(topRow);
    topRowLayout->setContentsMargins(0, 0, 0, 0);

    m_pageCombo = new QComboBox(topRow);
    m_pageCombo->setMinimumWidth(160);
    m_btnSelectHistos = new QPushButton(tr("Select Histograms..."), topRow);
    topRowLayout->addWidget(new QLabel(tr("Page:"), topRow));
    topRowLayout->addWidget(m_pageCombo);
    topRowLayout->addWidget(m_btnSelectHistos);
    topRowLayout->addStretch(1);
    vplots->addWidget(topRow);

    m_plotWidget = new HistoWidget(gbPlots);
    m_plotWidget->setMinimumHeight(480);
    vplots->addWidget(m_plotWidget, 1);

    connect(m_pageCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &OnlineAnalysisInterface::ShowPage);
    connect(m_btnSelectHistos, &QPushButton::clicked,
            this, &OnlineAnalysisInterface::ConfigureVisibleHistograms);
    return gbPlots;
}

QGroupBox *OnlineAnalysisInterface::BuildLogGroup(QWidget *parent)
{
    QGroupBox *gbLog = new QGroupBox(tr("System Log"), parent);
    QVBoxLayout *layout = new QVBoxLayout(gbLog);

    m_log = new QPlainTextEdit(gbLog);
    m_log->setReadOnly(true);
    m_log->setMaximumBlockCount(20000);
    m_log->setLineWrapMode(QPlainTextEdit::WidgetWidth);
    m_log->setMinimumHeight(40);
    layout->addWidget(m_log);
    return gbLog;
}

void OnlineAnalysisInterface::ConfigureMainSplitter(QSplitter *mainSplitter,
                                                    QSplitter *contentSplitter,
                                                    QWidget *plots,
                                                    QWidget *settings,
                                                    QWidget *log)
{
    contentSplitter->addWidget(plots);
    contentSplitter->addWidget(settings);
    contentSplitter->setSizes({4000, 1000});
    contentSplitter->setStretchFactor(0, 4);
    contentSplitter->setStretchFactor(1, 1);
    contentSplitter->setCollapsible(0, false);

    mainSplitter->addWidget(contentSplitter);
    mainSplitter->addWidget(log);
    mainSplitter->setStretchFactor(0, 5);
    mainSplitter->setStretchFactor(1, 1);
    mainSplitter->setCollapsible(0, false);
    mainSplitter->setCollapsible(1, false);
    mainSplitter->setSizes({850, 120});
}

void OnlineAnalysisInterface::ApplyStyle()
{
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
}

void OnlineAnalysisInterface::WireSignals()
{
    connect(m_btnBrowsePed, &QPushButton::clicked, this, &OnlineAnalysisInterface::BrowsePedestal);
    connect(m_btnBrowseCm,  &QPushButton::clicked, this, &OnlineAnalysisInterface::BrowseCommonMode);
    connect(m_btnBrowseDir, &QPushButton::clicked, this, &OnlineAnalysisInterface::BrowseRawDir);
    connect(m_btnBrowseOut, &QPushButton::clicked, this, &OnlineAnalysisInterface::BrowseOutDir);
    connect(m_btnStart,     &QPushButton::clicked, this, &OnlineAnalysisInterface::StartAnalysis);
}

////////////////////////////////////////////////////////////////////////////////
// browse buttons

void OnlineAnalysisInterface::BrowsePedestal()
{
    QString f = QFileDialog::getOpenFileName(this, tr("Pedestal file"),
            m_edPed->text().isEmpty() ? QString("database") : m_edPed->text());
    if(!f.isEmpty()) m_edPed->setText(f);
}

void OnlineAnalysisInterface::BrowseCommonMode()
{
    QString f = QFileDialog::getOpenFileName(this, tr("Common-mode file"),
            m_edCm->text().isEmpty() ? QString("database") : m_edCm->text());
    if(!f.isEmpty()) m_edCm->setText(f);
}

void OnlineAnalysisInterface::BrowseRawDir()
{
    QString d = QFileDialog::getExistingDirectory(this, tr("Raw data folder"),
            m_edRawDir->text().isEmpty() ? QString("./") : m_edRawDir->text());
    if(!d.isEmpty()) m_edRawDir->setText(d);
}

void OnlineAnalysisInterface::BrowseOutDir()
{
    QString d = QFileDialog::getExistingDirectory(this, tr("Output folder"),
            m_edOutDir->text().isEmpty() ? QString("./") : m_edOutDir->text());
    if(!d.isEmpty()) m_edOutDir->setText(d);
}

////////////////////////////////////////////////////////////////////////////////
// helpers

void OnlineAnalysisInterface::AppendLog(const QString &line)
{
    m_log->appendPlainText(line);
    QScrollBar *sb = m_log->verticalScrollBar();
    if(sb) sb->setValue(sb->maximum());
}

QString OnlineAnalysisInterface::ResolveReplayBinary() const
{
    // Try several anchors so the lookup works whether the GUI was launched
    // from the repo root (CWD has bin/), from Finder (CWD = /), or from any
    // other directory. Resolution order:
    //   1. CWD-relative (fast path; matches the old behaviour)
    //   2. Repo root we detected at construction time
    const QStringList anchors = { QStringLiteral("."), m_repoRoot };
    for(const QString &anchor : anchors) {
        if(anchor.isEmpty()) continue;
        const QFileInfo plain(QDir(anchor).filePath("bin/replay"));
        if(plain.exists() && plain.isExecutable())
            return plain.absoluteFilePath();
        const QFileInfo macApp(QDir(anchor).filePath(
                "bin/replay.app/Contents/MacOS/replay"));
        if(macApp.exists() && macApp.isExecutable())
            return macApp.absoluteFilePath();
    }
    return QString();
}

QString OnlineAnalysisInterface::DetectRepoRoot() const
{
    // The repo root contains setup_env.sh and the bin/ folder. Try CWD
    // first; if that doesn't look like a repo, walk up from the application
    // binary's directory looking for setup_env.sh.
    if(QFileInfo("./setup_env.sh").exists())
        return QDir::currentPath();

    QDir d(QCoreApplication::applicationDirPath());
    for(int i = 0; i < 6; ++i) {                 // bounded walk-up
        if(d.exists("setup_env.sh"))
            return d.absolutePath();
        if(!d.cdUp()) break;
    }
    return QDir::currentPath();                  // best-effort fallback
}

QString OnlineAnalysisInterface::ShellQuote(const QString &s)
{
    // Wrap in single quotes and escape any embedded ' as '\''. Safe for any
    // user-supplied string going into a bash -c command.
    QString out = s;
    out.replace(QStringLiteral("'"), QStringLiteral("'\\''"));
    return QStringLiteral("'") + out + QStringLiteral("'");
}

void OnlineAnalysisInterface::SetControlsEnabled(bool on)
{
    m_edPed->setEnabled(on);
    m_edCm->setEnabled(on);
    m_edRawDir->setEnabled(on);
    m_edOutDir->setEnabled(on);
    m_edPattern->setEnabled(on);
    m_edPrefix->setEnabled(on);
    m_spRun->setEnabled(on);
    m_spSplits->setEnabled(on);
    m_spCores->setEnabled(on);
    m_btnBrowsePed->setEnabled(on);
    m_btnBrowseCm->setEnabled(on);
    m_btnBrowseDir->setEnabled(on);
    m_btnBrowseOut->setEnabled(on);
    m_btnStart->setEnabled(on);
}

////////////////////////////////////////////////////////////////////////////////
// start: validate, glob, distribute, launch QProcess workers

void OnlineAnalysisInterface::StartAnalysis()
{
    AnalysisRequest request;
    if(!PrepareAnalysisRequest(request))
        return;

    // Keep the prior log so users can compare runs; just stamp a divider.
    AppendLog(QString(50, '=').prepend(QString()));
    AppendLog(QString("== run %1 ==").arg(request.run));
    ClearHistos();
    LogAnalysisPreamble(request);
    CreateWorkerProcesses(request);
    StartWorkerProcesses();
}

bool OnlineAnalysisInterface::PrepareAnalysisRequest(AnalysisRequest &request)
{
    request.ped = m_edPed->text().trimmed();
    request.cm = m_edCm->text().trimmed();
    request.rawDir = m_edRawDir->text().trimmed();
    request.outDir = m_edOutDir->text().trimmed();
    request.pattern = m_edPattern->text().trimmed();
    request.prefix = m_edPrefix->text().trimmed();
    request.run = m_spRun->value();
    request.nSplit = m_spSplits->value();
    request.nCore = m_spCores->value();

    if(request.ped.isEmpty() || request.cm.isEmpty()
       || request.rawDir.isEmpty() || request.outDir.isEmpty()
       || request.pattern.isEmpty() || request.prefix.isEmpty()) {
        QMessageBox::warning(this, tr("Online Analysis"),
                tr("Please fill in pedestal, common-mode, raw folder, "
                   "output folder, pattern, and prefix."));
        return false;
    }

    while(request.outDir.endsWith('/'))
        request.outDir.chop(1);

    if(!QFileInfo(request.ped).exists() || !QFileInfo(request.cm).exists()) {
        QMessageBox::warning(this, tr("Online Analysis"),
                tr("Pedestal or common-mode file does not exist."));
        return false;
    }
    if(!QFileInfo(request.rawDir).isDir()) {
        QMessageBox::warning(this, tr("Online Analysis"),
                tr("Raw data folder does not exist."));
        return false;
    }

    request.replayBin = ResolveReplayBinary();
    if(request.replayBin.isEmpty()) {
        QMessageBox::critical(this, tr("Online Analysis"),
                tr("replay binary not found (looked for ./bin/replay and the .app bundle)."));
        return false;
    }

    request.resolvedPattern = request.pattern;
    request.resolvedPattern.replace("{RUN}", QString::number(request.run));

    QDir rawDir(request.rawDir);
    QStringList files = rawDir.entryList({request.resolvedPattern},
                                         QDir::Files, QDir::Name);
    if(files.isEmpty()) {
        QMessageBox::warning(this, tr("Online Analysis"),
                tr("No files match %1 in %2")
                .arg(request.resolvedPattern, request.rawDir));
        return false;
    }

    for(const QString &file : files)
        request.absFiles << rawDir.absoluteFilePath(file);

    request.nFound = request.absFiles.size();
    if(request.nSplit <= 0 || request.nSplit > request.nFound)
        request.nSplit = request.nFound;
    if(request.nCore > request.nSplit)
        request.nCore = request.nSplit;
    if(request.nCore < 1)
        request.nCore = 1;
    request.nJob = (request.nSplit + request.nCore - 1) / request.nCore;

    if(!QDir().mkpath(request.outDir)) {
        QMessageBox::warning(this, tr("Online Analysis"),
                tr("Failed to create output folder: %1").arg(request.outDir));
        return false;
    }
    return true;
}

void OnlineAnalysisInterface::LogAnalysisPreamble(const AnalysisRequest &request)
{
    AppendLog(QString("Analyzing run %1: %2 splits across %3 core(s) (~%4 file(s) per core)")
              .arg(request.run).arg(request.nSplit)
              .arg(request.nCore).arg(request.nJob));
    AppendLog(QString("Replay binary: %1").arg(request.replayBin));
    AppendLog(QString("Pedestal:      %1").arg(request.ped));
    AppendLog(QString("Common mode:   %1").arg(request.cm));
    AppendLog(QString("Pattern:       %1   ->   %2 file(s) matched")
              .arg(request.resolvedPattern).arg(request.nFound));
    AppendLog(QString("Output folder: %1").arg(request.outDir));
    AppendLog(QString("Output prefix: %1").arg(request.prefix));
    AppendLog(QString::fromLatin1("--------------------------------------------------"));
}

QString OnlineAnalysisInterface::BuildWorkerCommand(const AnalysisRequest &request,
                                                    int core, int start,
                                                    int end) const
{
    // Every user-controlled string is shell-quoted before splicing into the
    // bash command so paths containing spaces, $, `, ; or ' cannot end up
    // being interpreted by the shell.
    QStringList quotedFiles;
    for(int i = start; i <= end; ++i)
        quotedFiles << ShellQuote(request.absFiles[i]);

    const QString cdLine = m_repoRoot.isEmpty()
        ? QString()
        : QString("cd %1 2>/dev/null; ").arg(ShellQuote(m_repoRoot));

    // - Cd to the repo root so the relative `./setup_env.sh` and any other
    //   relative paths inside it resolve regardless of QProcess CWD.
    // - Source setup_env.sh so dyld/ld can find libgem/libdecoder/etc.
    return QString(
        "%10"
        "[ -f ./setup_env.sh ] && . ./setup_env.sh >/dev/null 2>&1; "
        "files=( %1 ); "
        "for ((k=0; k<${#files[@]}; k++)); do "
        "  idx=$((%2 + k)); "
        "  out=%9/%3_run%4_split${idx}.root; "
        "  echo \"[core %5] split ${idx} -> ${out}\"; "
        "  %6 -c 0 -t 0 -z 1 -n -1 --tracking off "
        "     --pedestal %7 --common_mode %8 "
        "     --output_root_filename \"${out}\" \"${files[$k]}\"; "
        "done"
        )
        .arg(quotedFiles.join(' '))
        .arg(start)
        .arg(ShellQuote(request.prefix))
        .arg(request.run)
        .arg(core)
        .arg(ShellQuote(request.replayBin))
        .arg(ShellQuote(request.ped))
        .arg(ShellQuote(request.cm))
        .arg(ShellQuote(request.outDir))
        .arg(cdLine);
}

void OnlineAnalysisInterface::CreateWorkerProcesses(const AnalysisRequest &request)
{
    // Delete the previous run's QProcess children before dropping their
    // pointers, otherwise they linger as children of `this` forever.
    qDeleteAll(m_workers);
    m_workers.clear();
    m_activeWorkers = 0;
    m_workerFailures = 0;
    m_currentRun = request.run;
    m_currentPrefix = request.prefix;
    m_currentOutDir = request.outDir;

    for(int core = 0; core < request.nCore; ++core) {
        int start = core * request.nJob;
        int end = start + request.nJob - 1;
        if(end >= request.nSplit)
            end = request.nSplit - 1;
        if(start > end) continue;

        QProcess *p = new QProcess(this);
        p->setProgram("/bin/bash");
        p->setArguments({"-c", BuildWorkerCommand(request, core, start, end)});
        p->setProcessChannelMode(QProcess::MergedChannels);
        p->setProperty("core", core);
        p->setProperty("buf", QByteArray());   // per-process line buffer
        connect(p, &QProcess::readyReadStandardOutput,
                this, &OnlineAnalysisInterface::OnWorkerReadyRead);
        connect(p, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                this, &OnlineAnalysisInterface::OnWorkerFinished);
        m_workers.append(p);

        AppendLog(QString("CPU %1 will process splits %2 -> %3")
                  .arg(core).arg(start).arg(end));
    }
}

void OnlineAnalysisInterface::StartWorkerProcesses()
{
    if(m_workers.isEmpty()) {
        AppendLog("No work to do.");
        return;
    }

    SetControlsEnabled(false);
    m_activeWorkers = m_workers.size();
    for(QProcess *p : m_workers)
        p->start();
}

////////////////////////////////////////////////////////////////////////////////
// stream worker stdout/stderr into the log, prefixed with the core index

void OnlineAnalysisInterface::OnWorkerReadyRead()
{
    QProcess *p = qobject_cast<QProcess*>(sender());
    if(!p) return;
    const int core = p->property("core").toInt();

    // Append new bytes to the per-process buffer. Flush newline/carriage-return
    // delimited progress first, then flush the remaining tail immediately so
    // long-running replay output is visible in real time.
    QByteArray buf = p->property("buf").toByteArray();
    buf += p->readAllStandardOutput();
    buf.replace('\r', '\n');

    int sep;
    while((sep = buf.indexOf('\n')) >= 0) {
        const QByteArray line = buf.left(sep);
        buf.remove(0, sep + 1);
        if(line.isEmpty()) continue;
        AppendLog(QString("[core %1] %2")
                  .arg(core)
                  .arg(QString::fromLocal8Bit(line).trimmed()));
    }
    if(!buf.isEmpty()) {
        AppendLog(QString("[core %1] %2")
                  .arg(core)
                  .arg(QString::fromLocal8Bit(buf).trimmed()));
        buf.clear();
    }
    p->setProperty("buf", buf);
}

////////////////////////////////////////////////////////////////////////////////
// when all workers exit, hadd-merge the per-split ROOT files

void OnlineAnalysisInterface::OnWorkerFinished(int exitCode, int exitStatus)
{
    QProcess *p = qobject_cast<QProcess*>(sender());
    if(p) {
        const int core = p->property("core").toInt();
        // flush any tail bytes that didn't end with a newline
        const QByteArray tail = p->property("buf").toByteArray();
        if(!tail.isEmpty()) {
            AppendLog(QString("[core %1] %2")
                      .arg(core)
                      .arg(QString::fromLocal8Bit(tail).trimmed()));
        }
        const bool ok = (exitStatus == QProcess::NormalExit && exitCode == 0);
        if(!ok) {
            ++m_workerFailures;
            AppendLog(QString("[core %1] FAILED (exit code %2, status %3)")
                      .arg(core).arg(exitCode).arg(int(exitStatus)));
        } else {
            AppendLog(QString("[core %1] finished (exit %2)").arg(core).arg(exitCode));
        }
    }
    if(--m_activeWorkers > 0)
        return;

    if(m_workerFailures > 0) {
        AppendLog(QString("WARNING: %1 worker(s) failed; merge will run anyway, "
                          "output may be incomplete.").arg(m_workerFailures));
    }
    AppendLog("All replay jobs finished, merging ROOT files via hadd...");
    StartMergeProcess();
}

QString OnlineAnalysisInterface::BuildMergeCommand() const
{
    const QString outMain = QString("%1/%2_run%3.root")
                          .arg(m_currentOutDir).arg(m_currentPrefix)
                          .arg(m_currentRun);
    const QString outQc = QString("%1/%2_run%3.root_data_quality_check.root")
                        .arg(m_currentOutDir).arg(m_currentPrefix)
                        .arg(m_currentRun);

    const QString cdLine = m_repoRoot.isEmpty()
        ? QString()
        : QString("cd %1 2>/dev/null; ").arg(ShellQuote(m_repoRoot));

    // Every interpolated string is shell-quoted; the run number is a plain
    // int. Source setup_env.sh so hadd / ROOT libs resolve.
    return QString(
        "%6"
        "[ -f ./setup_env.sh ] && . ./setup_env.sh >/dev/null 2>&1; "
        "main_pattern=$(ls %5/%1_run%2_split*.root 2>/dev/null | grep -v data_quality_check); "
        "if [ -n \"$main_pattern\" ]; then "
        "  hadd -f %3 $main_pattern || exit 11; "
        "fi; "
        "qc_pattern=$(ls %5/%1_run%2_split*.root_data_quality_check.root 2>/dev/null); "
        "if [ -n \"$qc_pattern\" ]; then "
        "  hadd -f %4 $qc_pattern || exit 12; "
        "fi; "
        "rm -f %5/%1_run%2_split*.root "
        "      %5/%1_run%2_split*.root_data_quality_check.root; "
        "echo MERGE_OK"
        ).arg(ShellQuote(m_currentPrefix))
         .arg(m_currentRun)
         .arg(ShellQuote(outMain))
         .arg(ShellQuote(outQc))
         .arg(ShellQuote(m_currentOutDir))
         .arg(cdLine);
}

void OnlineAnalysisInterface::StartMergeProcess()
{
    if(m_mergeProc) { m_mergeProc->deleteLater(); m_mergeProc = nullptr; }
    m_mergeProc = new QProcess(this);
    m_mergeProc->setProgram("/bin/bash");
    m_mergeProc->setArguments({"-c", BuildMergeCommand()});
    m_mergeProc->setProcessChannelMode(QProcess::MergedChannels);
    m_mergeProc->setProperty("buf", QByteArray());
    connect(m_mergeProc, &QProcess::readyReadStandardOutput,
            this, &OnlineAnalysisInterface::OnMergeReadyRead);
    connect(m_mergeProc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &OnlineAnalysisInterface::OnMergeFinished);
    m_mergeProc->start();
}

void OnlineAnalysisInterface::OnMergeReadyRead()
{
    if(!m_mergeProc) return;
    QByteArray buf = m_mergeProc->property("buf").toByteArray();
    buf += m_mergeProc->readAllStandardOutput();
    buf.replace('\r', '\n');

    int sep;
    while((sep = buf.indexOf('\n')) >= 0) {
        const QByteArray line = buf.left(sep);
        buf.remove(0, sep + 1);
        if(line.isEmpty()) continue;
        AppendLog(QString("[merge] %1").arg(QString::fromLocal8Bit(line).trimmed()));
    }
    if(!buf.isEmpty()) {
        AppendLog(QString("[merge] %1").arg(QString::fromLocal8Bit(buf).trimmed()));
        buf.clear();
    }
    m_mergeProc->setProperty("buf", buf);
}

void OnlineAnalysisInterface::OnMergeFinished(int exitCode, int /*exitStatus*/)
{
    if(m_mergeProc) {
        const QByteArray tail = m_mergeProc->property("buf").toByteArray();
        if(!tail.isEmpty()) {
            AppendLog(QString("[merge] %1")
                      .arg(QString::fromLocal8Bit(tail).trimmed()));
        }
    }
    AppendLog(QString("[merge] finished (exit %1)").arg(exitCode));
    if(exitCode != 0) {
        AppendLog("Merge failed -- not plotting.");
        SetControlsEnabled(true);
        return;
    }
    Plot();
    SetControlsEnabled(true);
}

////////////////////////////////////////////////////////////////////////////////
// open the merged data-quality file and draw all TH1/TH2 histograms,
// 4-per-canvas (mirrors scripts/plot_quality_results.cpp).

void OnlineAnalysisInterface::Plot()
{
    const QString qcPath = DataQualityFilePath();
    if(!QFileInfo(qcPath).exists()) {
        AppendLog(QString("No data-quality file at %1 to plot.").arg(qcPath));
        return;
    }

    // Invariant: m_currentRun / m_currentPrefix / m_currentOutDir were set
    // together in CreateWorkerProcesses, so DataQualityFilePath()'s state
    // matches whichever run we just merged.
    AppendLog(QString("Plotting %1 ...").arg(qcPath));
    ClearHistos();
    if(!LoadHistograms(qcPath)) {
        AppendLog(QString("No histograms found in %1.").arg(qcPath));
        return;
    }

    LoadHistogramVisibilityConfig();
    RebuildVisibleHistogramList();
    SaveHistogramVisibilityConfig();

    PopulatePageCombo();
    const int nPages = m_pageCombo->count();
    if(nPages > 0) {
        m_pageCombo->setCurrentIndex(0);
        ShowPage(0);              // explicit because index was already 0
    } else if(m_plotWidget) {
        m_plotWidget->Clear();
    }
}

QString OnlineAnalysisInterface::DataQualityFilePath() const
{
    return QString("%1/%2_run%3.root_data_quality_check.root")
        .arg(m_currentOutDir).arg(m_currentPrefix).arg(m_currentRun);
}

bool OnlineAnalysisInterface::LoadHistograms(const QString &path)
{
    TFile *f = TFile::Open(path.toLocal8Bit().constData(), "READ");
    if(!f || f->IsZombie()) {
        AppendLog(QString("Failed to open %1").arg(path));
        if(f) f->Close();
        return false;
    }

    TIter keyList(f->GetListOfKeys());
    TKey *key = nullptr;
    while((key = static_cast<TKey*>(keyList()))) {
        TClass *cl = gROOT->GetClass(key->GetClassName());
        if(!cl || !cl->InheritsFrom("TH1")) continue;
        TH1 *h = dynamic_cast<TH1*>(key->ReadObj());
        if(!h) continue;
        h->SetDirectory(nullptr);    // detach so it survives file close
        m_histos.push_back(h);
    }
    f->Close();
    delete f;
    f = nullptr;
    // signal success only if at least one TH1 was loaded; lets callers
    // print a clear "no histograms" message instead of an empty plots tab.
    return !m_histos.empty();
}

QString OnlineAnalysisInterface::HistogramVisibilityConfigPath() const
{
    QDir root(m_repoRoot.isEmpty() ? QDir::currentPath() : m_repoRoot);
    return root.filePath("config/online_analysis_histogram_visibility.conf");
}

QString OnlineAnalysisInterface::EncodeConfigKey(const QString &key)
{
    return QString::fromLatin1(key.toUtf8().toPercentEncoding());
}

QString OnlineAnalysisInterface::DecodeConfigKey(const QString &key)
{
    return QString::fromUtf8(QByteArray::fromPercentEncoding(key.toLatin1()));
}

void OnlineAnalysisInterface::LoadHistogramVisibilityConfig()
{
    m_histoVisible.clear();

    QFile file(HistogramVisibilityConfigPath());
    if(!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return;

    QTextStream in(&file);
    while(!in.atEnd()) {
        const QString line = in.readLine().trimmed();
        if(line.isEmpty() || line.startsWith('#'))
            continue;

        const QStringList fields = line.split('\t');
        if(fields.size() < 2)
            continue;
        m_histoVisible[DecodeConfigKey(fields[0])] = (fields[1].trimmed() != "0");
    }
}

void OnlineAnalysisInterface::SaveHistogramVisibilityConfig() const
{
    const QString path = HistogramVisibilityConfigPath();
    QDir().mkpath(QFileInfo(path).absolutePath());

    QFile file(path);
    if(!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate))
        return;

    QTextStream out(&file);
    out << "# DO NOT EDIT THIS FILE, THIS IS AUTOMATICALLY GENERATED BY THE CODE\n";
    for(TH1 *h : m_histos) {
        const QString name = QString::fromUtf8(h->GetName());
        const bool checked = m_histoVisible.value(name, true);
        out << EncodeConfigKey(name) << '\t' << (checked ? 1 : 0) << '\n';
    }
}

void OnlineAnalysisInterface::RebuildVisibleHistogramList()
{
    m_visibleHistoIndices.clear();
    for(int i = 0; i < static_cast<int>(m_histos.size()); ++i) {
        const QString name = QString::fromUtf8(m_histos[i]->GetName());
        if(!m_histoVisible.contains(name))
            m_histoVisible[name] = true;
        if(m_histoVisible.value(name, true))
            m_visibleHistoIndices.push_back(i);
    }
}

void OnlineAnalysisInterface::PopulatePageCombo()
{
    m_pageCombo->blockSignals(true);
    m_pageCombo->clear();

    const int nHistos = static_cast<int>(m_visibleHistoIndices.size());
    const int nPages = (nHistos + 3) / 4;
    for(int p = 0; p < nPages; ++p)
        m_pageCombo->addItem(QString("page %1 / %2").arg(p + 1).arg(nPages));

    m_pageCombo->blockSignals(false);

    AppendLog(QString("Showing %1 of %2 histogram(s) across %3 page(s).")
              .arg(nHistos).arg(static_cast<int>(m_histos.size())).arg(nPages));
}

////////////////////////////////////////////////////////////////////////////////
// Draw one page of ROOT histograms with Qt-native graphics items. ROOT is only
// used as the histogram data container here; no ROOT canvas / Quartz drawing.

void OnlineAnalysisInterface::ShowPage(int idx)
{
    if(!m_plotWidget) return;
    if(m_visibleHistoIndices.empty()) { m_plotWidget->Clear(); return; }

    const int base = idx * 4;
    std::vector<HistoWidget::PlotData> plots;
    for(int i = 0; i < 4; ++i) {
        const int visibleIdx = base + i;
        if(visibleIdx >= static_cast<int>(m_visibleHistoIndices.size())) break;
        const int hi = m_visibleHistoIndices[visibleIdx];

        TH1 *h = m_histos[hi];
        HistoWidget::PlotData plot;
        plot.title = h->GetTitle() && std::string(h->GetTitle()).size() > 0
            ? std::string(h->GetTitle())
            : std::string(h->GetName());

        if(TH2 *h2 = dynamic_cast<TH2*>(h)) {
            plot.type = HistoWidget::PlotData::Plot2D;
            char buf[128];
            std::snprintf(buf, sizeof(buf), "Entries %.0f", h2->GetEntries());
            plot.stats.push_back(buf);
            std::snprintf(buf, sizeof(buf), "Mean x %.3g", h2->GetMean(1));
            plot.stats.push_back(buf);
            std::snprintf(buf, sizeof(buf), "Mean y %.3g", h2->GetMean(2));
            plot.stats.push_back(buf);
            std::snprintf(buf, sizeof(buf), "Std x %.3g", h2->GetStdDev(1));
            plot.stats.push_back(buf);
            std::snprintf(buf, sizeof(buf), "Std y %.3g", h2->GetStdDev(2));
            plot.stats.push_back(buf);
            plot.nx = h2->GetNbinsX();
            plot.ny = h2->GetNbinsY();
            plot.xMin = h2->GetXaxis()->GetXmin();
            plot.xMax = h2->GetXaxis()->GetXmax();
            plot.yMin = h2->GetYaxis()->GetXmin();
            plot.yMax = h2->GetYaxis()->GetXmax();
            plot.z.reserve(plot.nx * plot.ny);
            for(int iy = 1; iy <= plot.ny; ++iy) {
                for(int ix = 1; ix <= plot.nx; ++ix)
                    plot.z.push_back(h2->GetBinContent(ix, iy));
            }
        } else {
            plot.type = HistoWidget::PlotData::Plot1D;
            char buf[128];
            std::snprintf(buf, sizeof(buf), "Entries %.0f", h->GetEntries());
            plot.stats.push_back(buf);
            std::snprintf(buf, sizeof(buf), "Mean %.3g", h->GetMean());
            plot.stats.push_back(buf);
            std::snprintf(buf, sizeof(buf), "Std Dev %.3g", h->GetStdDev());
            plot.stats.push_back(buf);
            const int nBins = h->GetNbinsX();
            // Pass the real X-axis range so the renderer can show ADC units
            // / cluster counts instead of bin indices.
            plot.nx   = nBins;
            plot.xMin = h->GetXaxis()->GetXmin();
            plot.xMax = h->GetXaxis()->GetXmax();
            plot.y.reserve(nBins);
            for(int bin = 1; bin <= nBins; ++bin)
                plot.y.push_back(h->GetBinContent(bin));
        }

        plots.push_back(plot);
    }
    m_plotWidget->DrawCanvas(plots, 2, 2);
}

void OnlineAnalysisInterface::ConfigureVisibleHistograms()
{
    if(m_histos.empty()) {
        QMessageBox::information(this, tr("Histogram Selection"),
                tr("No histograms are loaded yet."));
        return;
    }

    QDialog dlg(this);
    dlg.setWindowTitle(tr("Histogram Selection"));
    dlg.resize(520, 650);

    QVBoxLayout *layout = new QVBoxLayout(&dlg);
    QLabel *label = new QLabel(tr("Checked histograms are shown in the GUI. "
                                  "All histograms remain in the ROOT files."), &dlg);
    label->setWordWrap(true);
    layout->addWidget(label);

    QListWidget *list = new QListWidget(&dlg);
    for(TH1 *h : m_histos) {
        const QString name = QString::fromUtf8(h->GetName());
        const QString title = QString::fromUtf8(h->GetTitle());
        const QString text = (!title.isEmpty() && title != name)
            ? QString("%1  --  %2").arg(name, title)
            : name;
        QListWidgetItem *item = new QListWidgetItem(text, list);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setData(Qt::UserRole, name);
        item->setCheckState(m_histoVisible.value(name, true)
                            ? Qt::Checked : Qt::Unchecked);
    }
    layout->addWidget(list, 1);

    QHBoxLayout *quickButtons = new QHBoxLayout();
    QPushButton *selectAll = new QPushButton(tr("Select All"), &dlg);
    QPushButton *selectNone = new QPushButton(tr("Select None"), &dlg);
    quickButtons->addWidget(selectAll);
    quickButtons->addWidget(selectNone);
    quickButtons->addStretch(1);
    layout->addLayout(quickButtons);

    connect(selectAll, &QPushButton::clicked, list, [list]() {
        for(int i = 0; i < list->count(); ++i)
            list->item(i)->setCheckState(Qt::Checked);
    });
    connect(selectNone, &QPushButton::clicked, list, [list]() {
        for(int i = 0; i < list->count(); ++i)
            list->item(i)->setCheckState(Qt::Unchecked);
    });

    QDialogButtonBox *buttons = new QDialogButtonBox(
            QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
    layout->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

    if(dlg.exec() != QDialog::Accepted)
        return;

    for(int i = 0; i < list->count(); ++i) {
        QListWidgetItem *item = list->item(i);
        const QString name = item->data(Qt::UserRole).toString();
        m_histoVisible[name] = (item->checkState() == Qt::Checked);
    }

    RebuildVisibleHistogramList();
    SaveHistogramVisibilityConfig();
    PopulatePageCombo();

    if(!m_visibleHistoIndices.empty()) {
        m_pageCombo->setCurrentIndex(0);
        ShowPage(0);
    } else {
        m_plotWidget->Clear();
    }
}

////////////////////////////////////////////////////////////////////////////////
// release any histograms we previously owned

void OnlineAnalysisInterface::ClearHistos()
{
    for(TH1 *h : m_histos) delete h;
    m_histos.clear();
    m_visibleHistoIndices.clear();
    if(m_pageCombo) {
        m_pageCombo->blockSignals(true);
        m_pageCombo->clear();
        m_pageCombo->blockSignals(false);
    }
    if(m_plotWidget) m_plotWidget->Clear();
}
