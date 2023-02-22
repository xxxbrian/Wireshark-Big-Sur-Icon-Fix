/* main_window_slots.cpp
 *
 * Wireshark - Network traffic analyzer
 * By Gerald Combs <gerald@wireshark.org>
 * Copyright 1998 Gerald Combs
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <config.h>

// Qt 5.5.0 + Visual C++ 2013
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4996)
#endif

#include "wireshark_main_window.h"

/*
 * The generated Ui_WiresharkMainWindow::setupUi() can grow larger than our configured limit,
 * so turn off -Wframe-larger-than= for ui_main_window.h.
 */
DIAG_OFF(frame-larger-than=)
#include <ui_wireshark_main_window.h>
DIAG_ON(frame-larger-than=)

#ifdef _WIN32
#include <windows.h>
#endif

#include "ui/dissect_opts.h"

#ifdef HAVE_LIBPCAP
#include "ui/capture.h"
#endif

#include "ui/commandline.h"

#include "ui/urls.h"

#include "epan/color_filters.h"
#include "epan/export_object.h"

#include "wsutil/file_util.h"
#include "wsutil/filesystem.h"
#include <wsutil/wslog.h>
#include <wsutil/ws_assert.h>

#include "epan/addr_resolv.h"
#include "epan/column.h"
#include "epan/dfilter/dfilter-macro.h"
#include "epan/conversation_filter.h"
#include "epan/epan_dissect.h"
#include "epan/filter_expressions.h"
#include "epan/prefs.h"
#include "epan/plugin_if.h"
#include "epan/uat.h"
#include "epan/uat-int.h"
#include "epan/value_string.h"

#ifdef HAVE_LUA
#include <epan/wslua/init_wslua.h>
#endif

#include "ui/alert_box.h"
#ifdef HAVE_LIBPCAP
#include "ui/capture_ui_utils.h"
#endif

#include "ui/capture_globals.h"
#include "ui/help_url.h"
#include "ui/main_statusbar.h"
#include "ui/preference_utils.h"
#include "ui/recent.h"
#include "ui/recent_utils.h"
#include "ui/ssl_key_export.h"
#include "ui/ws_ui_util.h"
#include "ui/all_files_wildcard.h"
#include "ui/qt/simple_dialog.h"

#include <ui/qt/utils/variant_pointer.h>
#include <ui/qt/widgets/drag_drop_toolbar.h>
#include "ui/qt/widgets/wireshark_file_dialog.h"

#ifdef HAVE_SOFTWARE_UPDATE
#include "ui/software_update.h"
#endif

#include "about_dialog.h"
#include "bluetooth_att_server_attributes_dialog.h"
#include "bluetooth_devices_dialog.h"
#include "bluetooth_hci_summary_dialog.h"
#include "capture_file_dialog.h"
#include "capture_file_properties_dialog.h"
#ifdef HAVE_LIBPCAP
#include "capture_options_dialog.h"
#endif
#include <ui/qt/utils/color_utils.h>
#include "coloring_rules_dialog.h"
#include "conversation_dialog.h"
#include "conversation_colorize_action.h"
#include "conversation_hash_tables_dialog.h"
#include "enabled_protocols_dialog.h"
#include "decode_as_dialog.h"
#include <ui/qt/widgets/display_filter_edit.h>
#include "display_filter_expression_dialog.h"
#include "dissector_tables_dialog.h"
#include "endpoint_dialog.h"
#include "expert_info_dialog.h"
#include "export_object_action.h"
#include "export_object_dialog.h"
#include "export_pdu_dialog.h"
#include "extcap_options_dialog.h"
#include "file_set_dialog.h"
#include "filter_action.h"
#include "filter_dialog.h"
#include "firewall_rules_dialog.h"
#include "funnel_statistics.h"
#include "gsm_map_summary_dialog.h"
#include "iax2_analysis_dialog.h"
#include "interface_toolbar.h"
#include "io_graph_dialog.h"
#include <ui/qt/widgets/additional_toolbar.h>
#include "lbm_stream_dialog.h"
#include "lbm_lbtrm_transport_dialog.h"
#include "lbm_lbtru_transport_dialog.h"
#include "lte_mac_statistics_dialog.h"
#include "lte_rlc_statistics_dialog.h"
#include "lte_rlc_graph_dialog.h"
#include "main_application.h"
#include "mtp3_summary_dialog.h"
#include "multicast_statistics_dialog.h"
#include "packet_comment_dialog.h"
#include "packet_diagram.h"
#include "packet_dialog.h"
#include "packet_list.h"
#include "credentials_dialog.h"
#include "preferences_dialog.h"
#include "print_dialog.h"
#include "profile_dialog.h"
#include "protocol_hierarchy_dialog.h"
#include <ui/qt/utils/qt_ui_utils.h>
#include "resolved_addresses_dialog.h"
#include "rpc_service_response_time_dialog.h"
#include "rtp_stream_dialog.h"
#include "rtp_analysis_dialog.h"
#include "sctp_all_assocs_dialog.h"
#include "sctp_assoc_analyse_dialog.h"
#include "sctp_graph_dialog.h"
#include "sequence_dialog.h"
#include "show_packet_bytes_dialog.h"
#include "stats_tree_dialog.h"
#include "strip_headers_dialog.h"
#include <ui/qt/utils/stock_icon.h>
#include "supported_protocols_dialog.h"
#include "tap_parameter_dialog.h"
#include "tcp_stream_dialog.h"
#include "time_shift_dialog.h"
#include "uat_dialog.h"
#include "voip_calls_dialog.h"
#include "wlan_statistics_dialog.h"
#include <ui/qt/widgets/wireless_timeline.h>

#include <functional>
#include <QClipboard>
#include <QFileInfo>
#include <QMessageBox>
#include <QMetaObject>
#include <QToolBar>
#include <QDesktopServices>
#include <QUrl>
#include <QMutex>

// XXX You must uncomment QT_WINEXTRAS_LIB lines in CMakeList.txt and
// cmakeconfig.h.in.
// #if defined(QT_WINEXTRAS_LIB)
// #include <QWinJumpList>
// #include <QWinJumpListCategory>
// #include <QWinJumpListItem>
// #endif

//
// Public slots
//

bool WiresharkMainWindow::openCaptureFile(QString cf_path, QString read_filter, unsigned int type, gboolean is_tempfile)
{
    QString file_name = "";
    dfilter_t *rfcode = NULL;
    gchar *err_msg;
    int err;
    gboolean name_param;
    gboolean ret = true;

    // was a file name given as function parameter?
    name_param = !cf_path.isEmpty();

    for (;;) {

        if (cf_path.isEmpty()) {
            CaptureFileDialog open_dlg(this, capture_file_.capFile());

            if (open_dlg.open(file_name, type, read_filter)) {
                cf_path = file_name;
            } else {
                ret = false;
                goto finish;
            }
        } else {
            this->welcome_page_->getInterfaceFrame()->showRunOnFile();
        }

        // TODO detect call from "cf_read" -> "update_progress_dlg"
        // ("capture_file_.capFile()->read_lock"), possibly queue opening the
        // file and return early to avoid the warning in testCaptureFileClose.

        QString before_what(tr(" before opening another file"));
        if (!testCaptureFileClose(before_what)) {
            ret = false;
            goto finish;
        }

        if (dfilter_compile(qUtf8Printable(read_filter), &rfcode, &err_msg)) {
            cf_set_rfcode(CaptureFile::globalCapFile(), rfcode);
        } else {
            /* Not valid.  Tell the user, and go back and run the file
               selection box again once they dismiss the alert. */
               //bad_dfilter_alert_box(top_level, read_filter->str);
            QMessageBox::warning(this, tr("Invalid Display Filter"),
                    QString("The filter expression ") +
                    read_filter +
                    QString(" isn't a valid display filter. (") +
                    err_msg + QString(")."),
                    QMessageBox::Ok);

            if (!name_param) {
                // go back to the selection dialogue only if the file
                // was selected from this dialogue
                cf_path.clear();
                continue;
            }
        }

        /* Make the file name available via MainWindow */
        setMwFileName(cf_path);

        /* Try to open the capture file. This closes the current file if it succeeds. */
        CaptureFile::globalCapFile()->window = this;
        if (cf_open(CaptureFile::globalCapFile(), qUtf8Printable(cf_path), type, is_tempfile, &err) != CF_OK) {
            /* We couldn't open it; don't dismiss the open dialog box,
               just leave it around so that the user can, after they
               dismiss the alert box popped up for the open error,
               try again. */
            CaptureFile::globalCapFile()->window = NULL;
            dfilter_free(rfcode);
            cf_path.clear();
            continue;
        }

        switch (cf_read(CaptureFile::globalCapFile(), FALSE)) {
        case CF_READ_OK:
        case CF_READ_ERROR:
            /* Just because we got an error, that doesn't mean we were unable
               to read any of the file; we handle what we could get from the
               file. */
            break;

        case CF_READ_ABORTED:
            /* The user bailed out of re-reading the capture file; the
               capture file has been closed - just free the capture file name
               string and return (without changing the last containing
               directory). */
            capture_file_.setCapFile(NULL);
            ret = false;
            goto finish;
        }
        break;
    }

    mainApp->setLastOpenDirFromFilename(cf_path);

    main_ui_->statusBar->showExpert();

finish:
#ifdef HAVE_LIBPCAP
    if (global_commandline_info.quit_after_cap)
        exit(0);
#endif
    return ret;
}

void WiresharkMainWindow::filterPackets(QString new_filter, bool force)
{
    cf_status_t cf_status;

    cf_status = cf_filter_packets(CaptureFile::globalCapFile(), new_filter.toUtf8().data(), force);

    if (cf_status == CF_OK) {
        if (new_filter.length() > 0) {
            int index = df_combo_box_->findText(new_filter);
            if (index == -1) {
                df_combo_box_->insertItem(0, new_filter);
                df_combo_box_->setCurrentIndex(0);
            } else {
                df_combo_box_->setCurrentIndex(index);
            }
        } else {
            df_combo_box_->lineEdit()->clear();
        }
        // Only after the display filter has been updated,
        // disable the arrow button
        emit displayFilterSuccess(true);
    } else {
        emit displayFilterSuccess(false);
    }
    if (packet_list_) {
        packet_list_->resetColumns();
    }
}

void WiresharkMainWindow::layoutToolbars()
{
    Qt::ToolButtonStyle tbstyle = Qt::ToolButtonIconOnly;
    switch (prefs.gui_toolbar_main_style) {
    case TB_STYLE_TEXT:
        tbstyle = Qt::ToolButtonTextOnly;
        break;
    case TB_STYLE_BOTH:
        tbstyle = Qt::ToolButtonTextUnderIcon;
        break;
    }

    main_ui_->mainToolBar->setToolButtonStyle(tbstyle);

    main_ui_->mainToolBar->setVisible(recent.main_toolbar_show);
    main_ui_->displayFilterToolBar->setVisible(recent.filter_toolbar_show);
#if defined(HAVE_LIBNL) && defined(HAVE_NL80211)
    main_ui_->wirelessToolBar->setVisible(recent.wireless_toolbar_show);
#endif
    main_ui_->statusBar->setVisible(recent.statusbar_show);

    foreach(QAction *action, main_ui_->menuInterfaceToolbars->actions()) {
        QToolBar *toolbar = action->data().value<QToolBar *>();
        if (g_list_find_custom(recent.interface_toolbars, action->text().toUtf8(), (GCompareFunc)strcmp)) {
            toolbar->setVisible(true);
        } else {
            toolbar->setVisible(false);
        }
    }

    QList<QToolBar *> toolbars = findChildren<QToolBar *>();
    foreach(QToolBar *bar, toolbars) {
        AdditionalToolBar *iftoolbar = dynamic_cast<AdditionalToolBar *>(bar);
        if (iftoolbar) {
            bool visible = false;
            if (g_list_find_custom(recent.gui_additional_toolbars, qUtf8Printable(iftoolbar->menuName()), (GCompareFunc)strcmp))
                visible = true;

            iftoolbar->setVisible(visible);

        }
    }
}

void WiresharkMainWindow::updatePreferenceActions()
{
    main_ui_->actionViewPacketList->setEnabled(prefs_has_layout_pane_content(layout_pane_content_plist));
    main_ui_->actionViewPacketDetails->setEnabled(prefs_has_layout_pane_content(layout_pane_content_pdetails));
    main_ui_->actionViewPacketBytes->setEnabled(prefs_has_layout_pane_content(layout_pane_content_pbytes));
    main_ui_->actionViewPacketDiagram->setEnabled(prefs_has_layout_pane_content(layout_pane_content_pdiagram));

    main_ui_->actionViewNameResolutionPhysical->setChecked(gbl_resolv_flags.mac_name);
    main_ui_->actionViewNameResolutionNetwork->setChecked(gbl_resolv_flags.network_name);
    main_ui_->actionViewNameResolutionTransport->setChecked(gbl_resolv_flags.transport_name);

    // Should this be a "recent" setting?
    main_ui_->actionGoAutoScroll->setChecked(prefs.capture_auto_scroll);
}

void WiresharkMainWindow::updateRecentActions()
{
    main_ui_->actionViewMainToolbar->setChecked(recent.main_toolbar_show);
    main_ui_->actionViewFilterToolbar->setChecked(recent.filter_toolbar_show);
    main_ui_->actionViewWirelessToolbar->setChecked(recent.wireless_toolbar_show);
    main_ui_->actionViewStatusBar->setChecked(recent.statusbar_show);
    main_ui_->actionViewPacketList->setChecked(recent.packet_list_show && prefs_has_layout_pane_content(layout_pane_content_plist));
    main_ui_->actionViewPacketDetails->setChecked(recent.tree_view_show && prefs_has_layout_pane_content(layout_pane_content_pdetails));
    main_ui_->actionViewPacketBytes->setChecked(recent.byte_view_show && prefs_has_layout_pane_content(layout_pane_content_pbytes));
    main_ui_->actionViewPacketDiagram->setChecked(recent.packet_diagram_show && prefs_has_layout_pane_content(layout_pane_content_pdiagram));

    foreach(QAction *action, main_ui_->menuInterfaceToolbars->actions()) {
        if (g_list_find_custom(recent.interface_toolbars, action->text().toUtf8(), (GCompareFunc)strcmp)) {
            action->setChecked(true);
        } else {
            action->setChecked(false);
        }
    }

    foreach(QAction * action, main_ui_->menuAdditionalToolbars->actions()) {
        ext_toolbar_t * toolbar = VariantPointer<ext_toolbar_t>::asPtr(action->data());
        bool checked = false;
        if (toolbar && g_list_find_custom(recent.gui_additional_toolbars, toolbar->name, (GCompareFunc)strcmp))
            checked = true;

        action->setChecked(checked);
    }

    foreach(QAction* tda, td_actions.keys()) {
        if (recent.gui_time_format == td_actions[tda]) {
            tda->setChecked(true);
        }
    }
    foreach(QAction* tpa, tp_actions.keys()) {
        if (recent.gui_time_precision == tp_actions[tpa]) {
            tpa->setChecked(true);
            break;
        }
    }
    main_ui_->actionViewTimeDisplaySecondsWithHoursAndMinutes->setChecked(recent.gui_seconds_format == TS_SECONDS_HOUR_MIN_SEC);

    main_ui_->actionViewColorizePacketList->setChecked(recent.packet_list_colorize);
}

// Don't connect to this directly. Connect to or emit fiterAction(...) instead.
void WiresharkMainWindow::queuedFilterAction(QString action_filter, FilterAction::Action action, FilterAction::ActionType type)
{
    QString cur_filter, new_filter;

    if (!df_combo_box_) return;
    cur_filter = df_combo_box_->lineEdit()->text();

    switch (type) {
    case FilterAction::ActionTypePlain:
        new_filter = action_filter;
        break;
    case FilterAction::ActionTypeAnd:
        if (cur_filter.length()) {
            new_filter = "(" + cur_filter + ") && (" + action_filter + ")";
        }
        else {
            new_filter = action_filter;
        }
        break;
    case FilterAction::ActionTypeOr:
        if (cur_filter.length()) {
            new_filter = "(" + cur_filter + ") || (" + action_filter + ")";
        } else {
            new_filter = action_filter;
        }
        break;
    case FilterAction::ActionTypeNot:
        new_filter = "!(" + action_filter + ")";
        break;
    case FilterAction::ActionTypeAndNot:
        if (cur_filter.length()) {
            new_filter = "(" + cur_filter + ") && !(" + action_filter + ")";
        } else {
            new_filter = "!(" + action_filter + ")";
        }
        break;
    case FilterAction::ActionTypeOrNot:
        if (cur_filter.length()) {
            new_filter = "(" + cur_filter + ") || !(" + action_filter + ")";
        } else {
            new_filter = "!(" + action_filter + ")";
        }
        break;
    default:
        ws_assert_not_reached();
        break;
    }

    switch (action) {
    case FilterAction::ActionApply:
        df_combo_box_->lineEdit()->setText(new_filter);
        df_combo_box_->applyDisplayFilter();
        break;
    case FilterAction::ActionColorize:
        colorizeWithFilter(new_filter.toUtf8());
        break;
    case FilterAction::ActionCopy:
        mainApp->clipboard()->setText(new_filter);
        break;
    case FilterAction::ActionFind:
        main_ui_->searchFrame->findFrameWithFilter(new_filter);
        break;
    case FilterAction::ActionPrepare:
        df_combo_box_->lineEdit()->setText(new_filter);
        df_combo_box_->lineEdit()->setFocus();
        break;
    case FilterAction::ActionWebLookup:
    {
        QString url = QString("https://www.google.com/search?q=") + new_filter;
        QDesktopServices::openUrl(QUrl(url));
        break;
    }
    default:
        ws_assert_not_reached();
        break;
    }
}

// Capture callbacks

#ifdef HAVE_LIBPCAP
void WiresharkMainWindow::captureCapturePrepared(capture_session *session) {
    setTitlebarForCaptureInProgress();

    setWindowIcon(mainApp->captureIcon());

    /* Disable menu items that make no sense if you're currently running
       a capture. */
    bool handle_toolbars = (session->session_will_restart ? false : true);
    setForCaptureInProgress(true, handle_toolbars, session->capture_opts->ifaces);
//    set_capture_if_dialog_for_capture_in_progress(TRUE);

//    /* Don't set up main window for a capture file. */
//    main_set_for_capture_file(FALSE);
    showCapture();
}

void WiresharkMainWindow::captureCaptureUpdateStarted(capture_session *session) {

    /* We've done this in "prepared" above, but it will be cleared while
       switching to the next multiple file. */
    setTitlebarForCaptureInProgress();

    bool handle_toolbars = (session->session_will_restart ? false : true);
    setForCaptureInProgress(true, handle_toolbars, session->capture_opts->ifaces);

    setForCapturedPackets(true);
}

void WiresharkMainWindow::captureCaptureUpdateFinished(capture_session *session) {

    /* The capture isn't stopping any more - it's stopped. */
    capture_stopping_ = false;

    /* Update the main window as appropriate */
    updateForUnsavedChanges();

    /* Enable menu items that make sense if you're not currently running
     a capture. */
    bool handle_toolbars = (session->session_will_restart ? false : true);
    setForCaptureInProgress(false, handle_toolbars);
    setMenusForCaptureFile();

    setWindowIcon(mainApp->normalIcon());
    popLiveCaptureInProgress();

    if (global_commandline_info.quit_after_cap) {
        // Command line asked us to quit after capturing.
        // Don't pop up a dialog to ask for unsaved files etc.
        exit(0);
    }
}

void WiresharkMainWindow::captureCaptureFixedFinished(capture_session *) {

    /* The capture isn't stopping any more - it's stopped. */
    capture_stopping_ = false;

    /* Enable menu items that make sense if you're not currently running
     a capture. */
    setForCaptureInProgress(false);
    /* There isn't a real capture_file structure yet, so just force disabling
       menu options.  They will "refresh" when the capture file is reloaded to
       display packets */
    setMenusForCaptureFile(true);

    setWindowIcon(mainApp->normalIcon());
    popLiveCaptureInProgress();

    if (global_commandline_info.quit_after_cap) {
        // Command line asked us to quit after capturing.
        // Don't pop up a dialog to ask for unsaved files etc.
        exit(0);
    }
}

void WiresharkMainWindow::captureCaptureFailed(capture_session *) {
    /* Capture isn't stopping any more. */
    capture_stopping_ = false;

    setForCaptureInProgress(false);
    showWelcome();

    // Reset expert information indicator
    main_ui_->statusBar->captureFileClosing();
    mainApp->popStatus(WiresharkApplication::FileStatus);

    setWindowIcon(mainApp->normalIcon());
    popLiveCaptureInProgress();

    if (global_commandline_info.quit_after_cap) {
        // Command line asked us to quit after capturing.
        // Don't pop up a dialog to ask for unsaved files etc.
        exit(0);
    }
}
#endif // HAVE_LIBPCAP

// Callbacks from cfile.c and file.c via CaptureFile::captureFileCallback

void WiresharkMainWindow::captureEventHandler(CaptureEvent ev)
{
    switch (ev.captureContext()) {

    case CaptureEvent::File:
        switch (ev.eventType()) {
        case CaptureEvent::Opened:
            captureFileOpened();
            break;
        case CaptureEvent::Closing:
            captureFileClosing();
            break;
        case CaptureEvent::Closed:
            captureFileClosed();
            break;
        case CaptureEvent::Started:
            captureFileReadStarted(tr("Loading"));
            break;
        case CaptureEvent::Finished:
            captureFileReadFinished();
            break;
        default:
            break;
        }
        break;

    case CaptureEvent::Reload:
        switch (ev.eventType()) {
        case CaptureEvent::Started:
            captureFileReadStarted(tr("Reloading"));
            break;
        case CaptureEvent::Finished:
            captureFileReadFinished();
            break;
        default:
            break;
        }
        break;

    case CaptureEvent::Rescan:
        switch (ev.eventType()) {
        case CaptureEvent::Started:
            setMenusForCaptureFile(true);
            captureFileReadStarted(tr("Rescanning"));
            break;
        case CaptureEvent::Finished:
            captureFileReadFinished();
            break;
        default:
            break;
        }
        break;

    case CaptureEvent::Retap:
        switch (ev.eventType()) {
        case CaptureEvent::Started:
            freeze();
            break;
        case CaptureEvent::Finished:
            thaw();
            break;
        case CaptureEvent::Flushed:
            draw_tap_listeners(FALSE);
            break;
        default:
            break;
        }
        break;

    case CaptureEvent::Merge:
        switch (ev.eventType()) {
        case CaptureEvent::Started:
            mainApp->popStatus(WiresharkApplication::FileStatus);
            mainApp->pushStatus(WiresharkApplication::FileStatus, tr("Merging files."), QString());
            break;
        case CaptureEvent::Finished:
            mainApp->popStatus(WiresharkApplication::FileStatus);
            break;
        default:
            break;
        }
        break;

    case CaptureEvent::Save:
        switch (ev.eventType()) {
        case CaptureEvent::Started:
        {
            QFileInfo file_info(ev.filePath());
            mainApp->popStatus(WiresharkApplication::FileStatus);
            mainApp->pushStatus(WiresharkApplication::FileStatus, tr("Saving %1…").arg(file_info.fileName()));
            break;
        }
        default:
            break;
        }
        break;

#ifdef HAVE_LIBPCAP
    case CaptureEvent::Capture:
        switch (ev.eventType()) {
        case CaptureEvent::Prepared:
            captureCapturePrepared(ev.capSession());
            break;
        case CaptureEvent::Stopping:
            capture_stopping_ = true;
            setMenusForCaptureStopping();
            break;
        case CaptureEvent::Failed:
            captureCaptureFailed(ev.capSession());
        default:
            break;
        }
        break;

    case CaptureEvent::Update:
        switch (ev.eventType()) {
        case CaptureEvent::Started:
            captureCaptureUpdateStarted(ev.capSession());
            break;
        case CaptureEvent::Finished:
            captureCaptureUpdateFinished(ev.capSession());
            break;
        default:
            break;
        }
        break;

    case CaptureEvent::Fixed:
        switch (ev.eventType()) {
        case CaptureEvent::Finished:
            captureCaptureFixedFinished(ev.capSession());
            break;
        default:
            break;
        }
        break;
#endif
    }
}

void WiresharkMainWindow::captureFileOpened() {
    if (capture_file_.window() != this) return;

    file_set_dialog_->fileOpened(capture_file_.capFile());
    setMenusForFileSet(true);
    emit setCaptureFile(capture_file_.capFile());
}

void WiresharkMainWindow::captureFileReadStarted(const QString &action) {
//    tap_param_dlg_update();

    /* Set up main window for a capture file. */
//    main_set_for_capture_file(TRUE);

    mainApp->popStatus(WiresharkApplication::FileStatus);
    QString msg = QString(tr("%1: %2")).arg(action).arg(capture_file_.fileName());
    QString msgtip = QString();
    mainApp->pushStatus(WiresharkApplication::FileStatus, msg, msgtip);
    showCapture();
    main_ui_->actionAnalyzeReloadLuaPlugins->setEnabled(false);
    main_ui_->wirelessTimelineWidget->captureFileReadStarted(capture_file_.capFile());
}

void WiresharkMainWindow::captureFileReadFinished() {
    if (!capture_file_.capFile()->is_tempfile && capture_file_.capFile()->filename) {
        /* Add this filename to the list of recent files in the "Recent Files" submenu */
        add_menu_recent_capture_file(capture_file_.capFile()->filename);

        /* Remember folder for next Open dialog and save it in recent */
        mainApp->setLastOpenDirFromFilename(capture_file_.capFile()->filename);
    }

    /* Update the appropriate parts of the main window. */
    updateForUnsavedChanges();

    /* enable wireless timeline if capture allows it */
    main_ui_->wirelessTimelineWidget->captureFileReadFinished();

    /* Enable menu items that make sense if you have some captured packets. */
    setForCapturedPackets(true);

    main_ui_->statusBar->setFileName(capture_file_);
    main_ui_->actionAnalyzeReloadLuaPlugins->setEnabled(true);

    packet_list_->captureFileReadFinished();

    emit setDissectedCaptureFile(capture_file_.capFile());
}

void WiresharkMainWindow::captureFileClosing() {
    setMenusForCaptureFile(true);
    setForCapturedPackets(false);
    setForCaptureInProgress(false);

    // Reset expert information indicator
    main_ui_->statusBar->captureFileClosing();
    main_ui_->searchFrame->animatedHide();
    main_ui_->goToFrame->animatedHide();
    // gtk_widget_show(expert_info_none);
    emit setCaptureFile(NULL);
    emit setDissectedCaptureFile(NULL);
}

void WiresharkMainWindow::captureFileClosed() {
    packets_bar_update();

    file_set_dialog_->fileClosed();
    setMenusForFileSet(false);
    setWindowModified(false);

    // Reset expert information indicator
    main_ui_->statusBar->captureFileClosing();
    mainApp->popStatus(WiresharkApplication::FileStatus);

    setWSWindowTitle();
    setWindowIcon(mainApp->normalIcon());
    setMenusForSelectedPacket();
    setMenusForSelectedTreeRow();

#ifdef HAVE_LIBPCAP
    if (!global_capture_opts.multi_files_on)
        showWelcome();
#endif
}

//
// Private slots
//

// ui/gtk/capture_dlg.c:start_capture_confirmed

void WiresharkMainWindow::startCapture() {
    startCapture(QStringList());
}

void WiresharkMainWindow::startCapture(QStringList interfaces _U_) {
#ifdef HAVE_LIBPCAP
    interface_options *interface_opts;
    guint i;
    interface_t *device;
    gboolean can_start_capture = TRUE;

    if (interfaces.count() > 0) {
        global_capture_opts.num_selected = 0;
        for (i = 0; i < global_capture_opts.all_ifaces->len; i++) {
            device = &g_array_index(global_capture_opts.all_ifaces, interface_t, i);

            if (interfaces.contains(device->name)) {
                device->selected = TRUE;
                global_capture_opts.num_selected++;
            }
            else {
                device->selected = FALSE;
            }
        }
    }

    /* did the user ever select a capture interface before? */
    if (global_capture_opts.num_selected == 0) {
        QString msg = QString(tr("No interface selected."));
        mainApp->pushStatus(WiresharkApplication::TemporaryStatus, msg);
        main_ui_->actionCaptureStart->setChecked(false);
        return;
    }

    for (i = 0; i < global_capture_opts.all_ifaces->len; i++) {
        device = &g_array_index(global_capture_opts.all_ifaces, interface_t, i);
        if (device->selected && (device->if_info.type == IF_EXTCAP)) {
            /* device is EXTCAP and is selected. Check if all mandatory
             * settings are set.
             */
            if (extcap_has_configuration(device->name, TRUE))
            {
                /* Request openning of extcap options dialog */
                QString device_name(device->name);
                emit showExtcapOptions(device_name, false);
                /* Cancel start of capture */
                can_start_capture = FALSE;
            }
        }
    }

    /* If some of extcap was not configured, do not start with the capture */
    if (!can_start_capture) {
        QString msg = QString(tr("Configure all extcaps before start of capture."));
        mainApp->pushStatus(WiresharkApplication::TemporaryStatus, msg);
        main_ui_->actionCaptureStart->setChecked(false);
        return;
    }

    // Ideally we should have disabled the start capture
    // toolbar buttons and menu items. This may not be the
    // case, e.g. with QtMacExtras.
    if (!capture_filter_valid_) {
        QString msg = QString(tr("Invalid capture filter."));
        mainApp->pushStatus(WiresharkApplication::TemporaryStatus, msg);
        main_ui_->actionCaptureStart->setChecked(false);
        return;
    }

    showCapture();

    /* XXX - we might need to init other pref data as well... */

    /* XXX - can this ever happen? */
    if (cap_session_.state != CAPTURE_STOPPED)
        return;

    /* close the currently loaded capture file */
    cf_close((capture_file *)cap_session_.cf);

    /* Copy the selected interfaces to the set of interfaces to use for
       this capture. */
    collect_ifaces(&global_capture_opts);

    CaptureFile::globalCapFile()->window = this;
    info_data_.ui.ui = this;
    if (capture_start(&global_capture_opts, NULL, &cap_session_, &info_data_,
                      main_window_update)) {
        capture_options *capture_opts = cap_session_.capture_opts;
        GString *interface_names;

        /* Add "interface name<live capture in progress>" on main status bar */
        interface_names = get_iface_list_string(capture_opts, 0);
        if (strlen(interface_names->str) > 0) {
            g_string_append(interface_names, ":");
        }
        g_string_append(interface_names, " ");

        mainApp->popStatus(WiresharkApplication::FileStatus);
        QString msg = QString("%1<live capture in progress>").arg(interface_names->str);
        QString msgtip = QString("to file: ");
        if (capture_opts->save_file)
            msgtip += capture_opts->save_file;
        mainApp->pushStatus(WiresharkApplication::FileStatus, msg, msgtip);
        g_string_free(interface_names, TRUE);

        /* The capture succeeded, which means the capture filter syntax is
         valid; add this capture filter to the recent capture filter list. */
        QByteArray filter_ba;
        for (i = 0; i < global_capture_opts.ifaces->len; i++) {
            interface_opts = &g_array_index(global_capture_opts.ifaces, interface_options, i);
            if (interface_opts->cfilter) {
                recent_add_cfilter(interface_opts->name, interface_opts->cfilter);
                if (filter_ba.isEmpty()) {
                    filter_ba = interface_opts->cfilter;
                } else {
                    /* Not the first selected interface; is its capture filter
                       the same as the one the other interfaces we've looked
                       at have? */
                    /* XXX: GCC 12.1 has a bogus warning at -O2 and higher
                     * even though the isEmpty() check guarantees that
                     * filter_ba.constData() is never NULL or empty.
                     */
#if WS_IS_AT_LEAST_GNUC_VERSION(12,1)
DIAG_OFF(stringop-overread)
#endif
                    if (strcmp(interface_opts->cfilter, filter_ba.constData()) != 0) {
#if WS_IS_AT_LEAST_GNUC_VERSION(12,1)
DIAG_ON(stringop-overread)
#endif
                        /* No, so not all selected interfaces have the same capture
                           filter. */
                        filter_ba.clear();
                    }
                }
            }
        }
        if (!filter_ba.isEmpty()) {
            recent_add_cfilter(NULL, filter_ba.constData());
        }
    } else {
        CaptureFile::globalCapFile()->window = NULL;
    }
#endif // HAVE_LIBPCAP
}

void WiresharkMainWindow::popLiveCaptureInProgress() {
    /* Pop the "<live capture in progress>" message off the status bar. */
    main_ui_->statusBar->setFileName(capture_file_);
}

void WiresharkMainWindow::stopCapture() {
//#ifdef HAVE_AIRPCAP
//  if (airpcap_if_active)
//    airpcap_set_toolbar_stop_capture(airpcap_if_active);
//#endif

#ifdef HAVE_LIBPCAP
    capture_stop(&cap_session_);
#endif // HAVE_LIBPCAP

}

// Keep focus rects from showing through the welcome screen. Primarily for
// macOS.
void WiresharkMainWindow::mainStackChanged(int)
{
    for (int i = 0; i < main_ui_->mainStack->count(); i++) {
        main_ui_->mainStack->widget(i)->setEnabled(i == main_ui_->mainStack->currentIndex());
    }
}

// XXX - Copied from ui/gtk/menus.c

/**
 * Add the capture filename (with an absolute path) to the "Recent Files" menu.
 */
// XXX - We should probably create a RecentFile class.
void WiresharkMainWindow::updateRecentCaptures() {
    QAction *ra;
    QMenu *recentMenu = main_ui_->menuOpenRecentCaptureFile;
    QString action_cf_name;

    if (!recentMenu) {
        return;
    }
    recentMenu->clear();

#if 0
#if defined(QT_WINEXTRAS_LIB)
     QWinJumpList recent_jl(this);
     QWinJumpListCategory *recent_jlc = recent_jl.recent();
     if (recent_jlc) {
         recent_jlc->clear();
         recent_jlc->setVisible(true);
     }
#endif
#endif
#if defined(Q_OS_MAC)
    if (!dock_menu_) {
        dock_menu_ = new QMenu();
        dock_menu_->setAsDockMenu();
    }
    dock_menu_->clear();
#endif

    /* Iterate through the actions in menuOpenRecentCaptureFile,
     * removing special items, a maybe duplicate entry and every item above count_max */
    int shortcut = Qt::Key_0;
    foreach(recent_item_status *ri, mainApp->recentItems()) {
        // Add the new item
        ra = new QAction(recentMenu);
        ra->setData(ri->filename);
        // XXX - Needs get_recent_item_status or equivalent
        ra->setEnabled(ri->accessible);
        recentMenu->insertAction(NULL, ra);
        action_cf_name = ra->data().toString();
        if (shortcut <= Qt::Key_9) {
            ra->setShortcut(Qt::META | (Qt::Key)shortcut);
            shortcut++;
        }
        ra->setText(action_cf_name);
        connect(ra, SIGNAL(triggered()), this, SLOT(recentActionTriggered()));

/* This is slow, at least on my VM here. The added links also open Wireshark
 * in a new window. It might make more sense to add a recent item when we
 * open a capture file. */
#if 0
#if defined(QT_WINEXTRAS_LIB)
     if (recent_jlc) {
         QFileInfo fi(ri->filename);
         QWinJumpListItem *jli = recent_jlc->addLink(
             fi.fileName(),
             QApplication::applicationFilePath(),
             QStringList() << "-r" << ri->filename
         );
         // XXX set icon
         jli->setWorkingDirectory(QDir::toNativeSeparators(QApplication::applicationDirPath()));
     }
#endif
#endif
#if defined(Q_OS_MAC)
        QAction *rda = new QAction(dock_menu_);
        QFileInfo fi(ri->filename);
        rda->setText(fi.fileName());
        dock_menu_->insertAction(NULL, rda);
        connect(rda, SIGNAL(triggered()), ra, SLOT(trigger()));
#endif
    }

    if (recentMenu->actions().count() > 0) {
        // Separator + "Clear"
        // XXX - Do we really need this?
        ra = new QAction(recentMenu);
        ra->setSeparator(true);
        recentMenu->insertAction(NULL, ra);

        ra = new QAction(recentMenu);
        ra->setText(tr("Clear Menu"));
        recentMenu->insertAction(NULL, ra);
        connect(ra, SIGNAL(triggered()), mainApp, SLOT(clearRecentCaptures()));
    } else {
        if (main_ui_->actionDummyNoFilesFound) {
            recentMenu->addAction(main_ui_->actionDummyNoFilesFound);
        }
    }
}

void WiresharkMainWindow::recentActionTriggered() {
    QAction *ra = qobject_cast<QAction*>(sender());

    if (ra) {
        QString cfPath = ra->data().toString();
        openCaptureFile(cfPath);
    }
}

QString WiresharkMainWindow::commentToMenuText(QString text, int max_len)
{
    text = text.trimmed().replace(QRegularExpression("(\\r?\\n|\\r\\n?)+"), " ");
    if (text.size() > 0) {
        if (text.size() > max_len) {
            text.truncate(max_len);
            text += "…";
        }
    }
    else {
        text = tr("(empty comment)", "placeholder for empty comment");
    }
    return text;
}

void WiresharkMainWindow::setEditCommentsMenu()
{
    main_ui_->menuPacketComment->clear();
    QAction *action = main_ui_->menuPacketComment->addAction(tr("Add New Comment…"));
    connect(action, &QAction::triggered, this, &WiresharkMainWindow::addPacketComment);
    action->setShortcut(QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_C));
    if (selectedRows().count() == 1) {
        const int thisRow = selectedRows().first();
        frame_data * current_frame = frameDataForRow(thisRow);
        wtap_block_t pkt_block = cf_get_packet_block(capture_file_.capFile(), current_frame);
        guint nComments = wtap_block_count_option(pkt_block, OPT_COMMENT);
        if (nComments > 0) {
            main_ui_->menuPacketComment->addSeparator();
            for (guint i = 0; i < nComments; i++) {
                QString comment = packet_list_->getPacketComment(i);
                comment = this->commentToMenuText(comment);
                action = main_ui_->menuPacketComment->addAction(tr("Edit \"%1\"", "edit packet comment").arg(comment));
                connect(action, &QAction::triggered, this, &WiresharkMainWindow::editPacketComment);
                action->setData(i);
            }

            main_ui_->menuPacketComment->addSeparator();
            for (guint i = 0; i < nComments; i++) {
                QString comment = packet_list_->getPacketComment(i);
                comment = this->commentToMenuText(comment);
                action = main_ui_->menuPacketComment->addAction(tr("Delete \"%1\"", "delete packet comment").arg(comment));
                connect(action, &QAction::triggered, this, &WiresharkMainWindow::deletePacketComment);
                action->setData(i);
            }
            main_ui_->menuPacketComment->addSeparator();
            action = main_ui_->menuPacketComment->addAction(tr("Delete packet comments"));
            connect(action, &QAction::triggered, this, &WiresharkMainWindow::deleteCommentsFromPackets);
        }
        wtap_block_unref(pkt_block);
    }
    if (selectedRows().count() > 1) {
        main_ui_->menuPacketComment->addSeparator();
        action = main_ui_->menuPacketComment->addAction(tr("Delete comments from %n packet(s)", nullptr, static_cast<int>(selectedRows().count())));
        connect(action, &QAction::triggered, this, &WiresharkMainWindow::deleteCommentsFromPackets);
    }
}

void WiresharkMainWindow::setMenusForSelectedPacket()
{
    gboolean is_ip = FALSE, is_tcp = FALSE, is_udp = FALSE, is_dccp = FALSE, is_sctp = FALSE, is_tls = FALSE, is_rtp = FALSE, is_lte_rlc = FALSE,
             is_http = FALSE, is_http2 = FALSE, is_quic = FALSE, is_sip = FALSE, is_exported_pdu = FALSE;

    /* Making the menu context-sensitive allows for easier selection of the
       desired item and has the added benefit, with large captures, of
       avoiding needless looping through huge lists for marked, ignored,
       or time-referenced packets. */

    /* We have one or more items in the packet list */
    bool have_frames = false;
    /* A frame is selected */
    bool frame_selected = false;
    bool multi_selection = false;
    /* A visible packet comes after this one in the selection history */
    bool next_selection_history = false;
    /* A visible packet comes before this one in the selection history */
    bool previous_selection_history = false;
    /* We have marked frames.  (XXX - why check frame_selected?) */
    bool have_marked = false;
    /* We have a marked frame other than the current frame (i.e.,
       we have at least one marked frame, and either there's more
       than one marked frame or the current frame isn't marked). */
    bool another_is_marked = false;
    /* One or more frames are hidden by a display filter */
    bool have_filtered = false;
    /* One or more frames have been ignored */
    bool have_ignored = false;
    bool have_time_ref = false;
    /* We have a time reference frame other than the current frame (i.e.,
       we have at least one time reference frame, and either there's more
       than one time reference frame or the current frame isn't a
       time reference frame). (XXX - why check frame_selected?) */
    bool another_is_time_ref = false;

    QList<QAction *> cc_actions = QList<QAction *>()
            << main_ui_->actionViewColorizeConversation1 << main_ui_->actionViewColorizeConversation2
            << main_ui_->actionViewColorizeConversation3 << main_ui_->actionViewColorizeConversation4
            << main_ui_->actionViewColorizeConversation5 << main_ui_->actionViewColorizeConversation6
            << main_ui_->actionViewColorizeConversation7 << main_ui_->actionViewColorizeConversation8
            << main_ui_->actionViewColorizeConversation9 << main_ui_->actionViewColorizeConversation10;

    if (capture_file_.capFile()) {
        QList<int> rows = selectedRows();
        frame_data * current_frame = 0;
        if (rows.count() > 0)
            current_frame = frameDataForRow(rows.at(0));

        frame_selected = rows.count() == 1;
        if (packet_list_->multiSelectActive())
        {
            frame_selected = false;
            multi_selection = true;
        }
        next_selection_history = packet_list_->haveNextHistory();
        previous_selection_history = packet_list_->havePreviousHistory();
        have_frames = capture_file_.capFile()->count > 0;
        have_marked = capture_file_.capFile()->marked_count > 0;
        another_is_marked = have_marked && rows.count() <= 1 &&
                !(capture_file_.capFile()->marked_count == 1 && frame_selected && current_frame->marked);
        have_filtered = capture_file_.capFile()->displayed_count > 0 && capture_file_.capFile()->displayed_count != capture_file_.capFile()->count;
        have_ignored = capture_file_.capFile()->ignored_count > 0;
        have_time_ref = capture_file_.capFile()->ref_time_count > 0;
        another_is_time_ref = have_time_ref && rows.count() <= 1 &&
                !(capture_file_.capFile()->ref_time_count == 1 && frame_selected && current_frame->ref_time);

        if (capture_file_.capFile()->edt && ! multi_selection)
        {
            proto_get_frame_protocols(capture_file_.capFile()->edt->pi.layers,
                                      &is_ip, &is_tcp, &is_udp, &is_sctp,
                                      &is_tls, &is_rtp, &is_lte_rlc);
            is_dccp = proto_is_frame_protocol(capture_file_.capFile()->edt->pi.layers, "dccp");
            is_http = proto_is_frame_protocol(capture_file_.capFile()->edt->pi.layers, "http");
            is_http2 = proto_is_frame_protocol(capture_file_.capFile()->edt->pi.layers, "http2");
            /* TODO: to follow a QUIC stream we need a *decrypted* QUIC connection, i.e. checking for "quic" in the protocol stack is not enough */
            is_quic = proto_is_frame_protocol(capture_file_.capFile()->edt->pi.layers, "quic");
            is_sip = proto_is_frame_protocol(capture_file_.capFile()->edt->pi.layers, "sip");
            is_exported_pdu = proto_is_frame_protocol(capture_file_.capFile()->edt->pi.layers, "exported_pdu");
            /* For Exported PDU there is a tag inserting IP addresses into the SRC and DST columns */
            if (is_exported_pdu &&
               (capture_file_.capFile()->edt->pi.net_src.type == AT_IPv4 || capture_file_.capFile()->edt->pi.net_src.type == AT_IPv6) &&
               (capture_file_.capFile()->edt->pi.net_dst.type == AT_IPv4 || capture_file_.capFile()->edt->pi.net_dst.type == AT_IPv6)) {
                is_ip = TRUE;
            }
        }
    }

    main_ui_->actionEditMarkPacket->setText(tr("&Mark/Unmark Packet(s)", "", static_cast<int>(selectedRows().count())));
    main_ui_->actionEditIgnorePacket->setText(tr("&Ignore/Unignore Packet(s)", "", static_cast<int>(selectedRows().count())));

    main_ui_->actionCopyListAsText->setEnabled(selectedRows().count() > 0);
    main_ui_->actionCopyListAsCSV->setEnabled(selectedRows().count() > 0);
    main_ui_->actionCopyListAsYAML->setEnabled(selectedRows().count() > 0);

    main_ui_->actionEditMarkPacket->setEnabled(frame_selected || multi_selection);
    main_ui_->actionEditMarkAllDisplayed->setEnabled(have_frames);
    /* Unlike un-ignore, do not allow unmark of all frames when no frames are displayed  */
    main_ui_->actionEditUnmarkAllDisplayed->setEnabled(have_marked);
    main_ui_->actionEditNextMark->setEnabled(another_is_marked);
    main_ui_->actionEditPreviousMark->setEnabled(another_is_marked);

    GArray * linkTypes = Q_NULLPTR;
    if (capture_file_.capFile() && capture_file_.capFile()->linktypes)
        linkTypes = capture_file_.capFile()->linktypes;

    bool enableEditComments = linkTypes && wtap_dump_can_write(capture_file_.capFile()->linktypes, WTAP_COMMENT_PER_PACKET);
    main_ui_->menuPacketComment->setEnabled(enableEditComments && selectedRows().count() > 0);
    main_ui_->actionDeleteAllPacketComments->setEnabled(enableEditComments);

    main_ui_->actionEditIgnorePacket->setEnabled(frame_selected || multi_selection);
    main_ui_->actionEditIgnoreAllDisplayed->setEnabled(have_filtered);
    /* Allow un-ignore of all frames even with no frames currently displayed */
    main_ui_->actionEditUnignoreAllDisplayed->setEnabled(have_ignored);

    main_ui_->actionEditSetTimeReference->setEnabled(frame_selected);
    main_ui_->actionEditUnsetAllTimeReferences->setEnabled(have_time_ref);
    main_ui_->actionEditNextTimeReference->setEnabled(another_is_time_ref);
    main_ui_->actionEditPreviousTimeReference->setEnabled(another_is_time_ref);
    main_ui_->actionEditTimeShift->setEnabled(have_frames);

    main_ui_->actionGoGoToLinkedPacket->setEnabled(false);
    main_ui_->actionGoNextHistoryPacket->setEnabled(next_selection_history);
    main_ui_->actionGoPreviousHistoryPacket->setEnabled(previous_selection_history);

    main_ui_->actionAnalyzeFollowTCPStream->setEnabled(is_tcp);
    main_ui_->actionAnalyzeFollowUDPStream->setEnabled(is_udp);
    main_ui_->actionAnalyzeFollowDCCPStream->setEnabled(is_dccp);
    main_ui_->actionAnalyzeFollowTLSStream->setEnabled(is_tls && !is_quic);
    main_ui_->actionAnalyzeFollowHTTPStream->setEnabled(is_http);
    main_ui_->actionAnalyzeFollowHTTP2Stream->setEnabled(is_http2);
    main_ui_->actionAnalyzeFollowQUICStream->setEnabled(is_quic);
    main_ui_->actionAnalyzeFollowSIPCall->setEnabled(is_sip);

    foreach(QAction *cc_action, cc_actions) {
        cc_action->setEnabled(frame_selected);
    }
    main_ui_->actionViewColorizeNewColoringRule->setEnabled(frame_selected);

    main_ui_->actionViewColorizeResetColorization->setEnabled(tmp_color_filters_used());

    main_ui_->actionViewShowPacketInNewWindow->setEnabled(frame_selected);
    main_ui_->actionViewEditResolvedName->setEnabled(frame_selected && is_ip);

    emit packetInfoChanged(capture_file_.packetInfo());

//    set_menu_sensitivity(ui_manager_main_menubar, "/Menubar/ViewMenu/NameResolution/ResolveName",
//                         frame_selected && (gbl_resolv_flags.mac_name || gbl_resolv_flags.network_name ||
//                                            gbl_resolv_flags.transport_name));

    main_ui_->actionToolsFirewallAclRules->setEnabled(frame_selected);

    main_ui_->actionStatisticsTcpStreamRoundTripTime->setEnabled(is_tcp);
    main_ui_->actionStatisticsTcpStreamStevens->setEnabled(is_tcp);
    main_ui_->actionStatisticsTcpStreamTcptrace->setEnabled(is_tcp);
    main_ui_->actionStatisticsTcpStreamThroughput->setEnabled(is_tcp);
    main_ui_->actionStatisticsTcpStreamWindowScaling->setEnabled(is_tcp);

    main_ui_->actionSCTPAnalyseThisAssociation->setEnabled(is_sctp);
    main_ui_->actionSCTPShowAllAssociations->setEnabled(is_sctp);
    main_ui_->actionSCTPFilterThisAssociation->setEnabled(is_sctp);
    main_ui_->actionTelephonyRtpStreamAnalysis->setEnabled(is_rtp);
    main_ui_->actionTelephonyRtpPlayer->setEnabled(is_rtp);
    main_ui_->actionTelephonyLteRlcGraph->setEnabled(is_lte_rlc);
}

void WiresharkMainWindow::setMenusForSelectedTreeRow(FieldInformation *finfo) {

    bool can_match_selected = false;
    bool is_framenum = false;
    bool have_field_info = false;
    bool have_subtree = false;
    bool can_open_url = false;
    bool have_packet_bytes = false;
    QByteArray field_filter;
    int field_id = -1;

    field_info * fi = 0;
    if (finfo)
        fi = finfo->fieldInfo();

    if (capture_file_.capFile()) {
        capture_file_.capFile()->finfo_selected = fi;

        if (fi && fi->tree_type != -1) {
            have_subtree = true;
        }

        if (fi && fi->ds_tvb && (fi->length > 0)) {
            have_packet_bytes = true;
        }
    }

    if (capture_file_.capFile() != NULL && fi != NULL) {
        header_field_info *hfinfo = fi->hfinfo;
        int linked_frame = -1;

        have_field_info = true;
        can_match_selected = proto_can_match_selected(capture_file_.capFile()->finfo_selected, capture_file_.capFile()->edt);
        if (hfinfo && hfinfo->type == FT_FRAMENUM) {
            is_framenum = true;
            linked_frame = fvalue_get_uinteger(&fi->value);
        }

        char *tmp_field = proto_construct_match_selected_string(fi, capture_file_.capFile()->edt);
        field_filter = tmp_field;
        wmem_free(NULL, tmp_field);

        field_id = fi->hfinfo->id;
        /* if the selected field isn't a protocol, get its parent */
        if (!proto_registrar_is_protocol(field_id)) {
            field_id = proto_registrar_get_parent(fi->hfinfo->id);
        }

        if (field_id >= 0) {
            can_open_url = true;
            main_ui_->actionContextWikiProtocolPage->setData(field_id);
            main_ui_->actionContextFilterFieldReference->setData(field_id);
        } else {
            main_ui_->actionContextWikiProtocolPage->setData(QVariant());
            main_ui_->actionContextFilterFieldReference->setData(QVariant());
        }

        if (linked_frame > 0) {
            main_ui_->actionGoGoToLinkedPacket->setData(linked_frame);
        } else {
            main_ui_->actionGoGoToLinkedPacket->setData(QVariant());
        }
    }

    // Always enable / disable the following items.
    main_ui_->actionFileExportPacketBytes->setEnabled(have_field_info);

    main_ui_->actionCopyAllVisibleItems->setEnabled(capture_file_.capFile() != NULL && ! packet_list_->multiSelectActive());
    main_ui_->actionCopyAllVisibleSelectedTreeItems->setEnabled(can_match_selected);
    main_ui_->actionEditCopyDescription->setEnabled(can_match_selected);
    main_ui_->actionEditCopyFieldName->setEnabled(can_match_selected);
    main_ui_->actionEditCopyValue->setEnabled(can_match_selected);
    main_ui_->actionEditCopyAsFilter->setEnabled(can_match_selected);

    main_ui_->actionAnalyzeShowPacketBytes->setEnabled(have_packet_bytes);
    main_ui_->actionFileExportPacketBytes->setEnabled(have_packet_bytes);

    main_ui_->actionViewExpandSubtrees->setEnabled(have_subtree);
    main_ui_->actionViewCollapseSubtrees->setEnabled(have_subtree);

    main_ui_->actionGoGoToLinkedPacket->setEnabled(is_framenum);

    main_ui_->actionAnalyzeCreateAColumn->setEnabled(can_match_selected);

    main_ui_->actionContextShowLinkedPacketInNewWindow->setEnabled(is_framenum);

    main_ui_->actionContextWikiProtocolPage->setEnabled(can_open_url);
    main_ui_->actionContextFilterFieldReference->setEnabled(can_open_url);


// Only enable / disable the following items if we have focus so that we
// don't clobber anything we may have set in setMenusForSelectedPacket.
    if (!proto_tree_ || !proto_tree_->hasFocus()) return;

    emit packetInfoChanged(capture_file_.packetInfo());
    emit fieldFilterChanged(field_filter);

    //    set_menu_sensitivity(ui_manager_tree_view_menu, "/TreeViewPopup/ResolveName",
    //                         frame_selected && (gbl_resolv_flags.mac_name || gbl_resolv_flags.network_name ||
    //                                            gbl_resolv_flags.transport_name));

}

void WiresharkMainWindow::interfaceSelectionChanged()
{
#ifdef HAVE_LIBPCAP
    // XXX This doesn't disable the toolbar button when using
    // QtMacExtras.
    if (global_capture_opts.num_selected > 0 && capture_filter_valid_) {
        main_ui_->actionCaptureStart->setEnabled(true);
    } else {
        main_ui_->actionCaptureStart->setEnabled(false);
    }
#endif // HAVE_LIBPCAP
}

void WiresharkMainWindow::captureFilterSyntaxChanged(bool valid)
{
    capture_filter_valid_ = valid;
    interfaceSelectionChanged();
}

void WiresharkMainWindow::startInterfaceCapture(bool valid, const QString capture_filter)
{
    capture_filter_valid_ = valid;
    welcome_page_->setCaptureFilter(capture_filter);
    QString before_what(tr(" before starting a new capture"));
    if (testCaptureFileClose(before_what)) {
        // The interface tree will update the selected interfaces via its timer
        // so no need to do anything here.
        startCapture(QStringList());
    }
}

void WiresharkMainWindow::applyGlobalCommandLineOptions()
{
    if (global_dissect_options.time_format != TS_NOT_SET) {
        foreach(QAction* tda, td_actions.keys()) {
            if (global_dissect_options.time_format == td_actions[tda]) {
                tda->setChecked(true);
                recent.gui_time_format = global_dissect_options.time_format;
                timestamp_set_type(global_dissect_options.time_format);
                break;
            }
        }
    }
    if (global_commandline_info.full_screen) {
        this->showFullScreen();
    }
}

void WiresharkMainWindow::redissectPackets()
{
    if (capture_file_.capFile()) {
        cf_redissect_packets(capture_file_.capFile());
        main_ui_->statusBar->expertUpdate();
    }

    proto_free_deregistered_fields();
}

void WiresharkMainWindow::checkDisplayFilter()
{
    if (!df_combo_box_->checkDisplayFilter()) {
        g_free(CaptureFile::globalCapFile()->dfilter);
        CaptureFile::globalCapFile()->dfilter = NULL;
    }
}

void WiresharkMainWindow::fieldsChanged()
{
    gchar *err_msg = NULL;
    if (!color_filters_reload(&err_msg, color_filter_add_cb)) {
        simple_dialog(ESD_TYPE_ERROR, ESD_BTN_OK, "%s", err_msg);
        g_free(err_msg);
    }
    tap_listeners_dfilter_recompile();

    emit checkDisplayFilter();

    if (have_custom_cols(&CaptureFile::globalCapFile()->cinfo)) {
        // Recreate packet list columns according to new/changed/deleted fields
        packet_list_->fieldsChanged(CaptureFile::globalCapFile());
    }

    emit reloadFields();
}

void WiresharkMainWindow::reloadLuaPlugins()
{
#ifdef HAVE_LUA
    if (mainApp->isReloadingLua())
        return;

    gboolean uses_lua_filehandler = FALSE;

    if (capture_file_.capFile()) {
        // Check if the current capture file is opened with a Lua FileHandler
        capture_file *cf = capture_file_.capFile();
        uses_lua_filehandler = wtap_uses_lua_filehandler(cf->provider.wth);

        if (uses_lua_filehandler && cf->unsaved_changes) {
            // Prompt to save the file before reloading, in case the FileHandler has changed
            QString before_what(tr(" before reloading Lua plugins"));
            if (!testCaptureFileClose(before_what, Reload)) {
                return;
            }
        }
    }

    mainApp->setReloadingLua(true);

    wslua_reload_plugins(NULL, NULL);
    funnel_statistics_reload_menus();
    reloadDynamicMenus();
    closePacketDialogs();

    // Preferences may have been deleted so close all widgets using prefs
    main_ui_->preferenceEditorFrame->animatedHide();

    mainApp->readConfigurationFiles(true);
    commandline_options_reapply();

    fieldsChanged();
    prefs_apply_all();

    if (uses_lua_filehandler) {
        // Reload the file in case the FileHandler has changed
        if (cf_reload(capture_file_.capFile()) != CF_OK) {
            cf_close(capture_file_.capFile());
        }
        proto_free_deregistered_fields();
    } else {
        redissectPackets();
    }

    mainApp->setReloadingLua(false);
    SimpleDialog::displayQueuedMessages();
#endif
}

void WiresharkMainWindow::showAccordionFrame(AccordionFrame *show_frame, bool toggle)
{
    QList<AccordionFrame *>frame_list = QList<AccordionFrame *>()
            << main_ui_->goToFrame << main_ui_->searchFrame
            << main_ui_->addressEditorFrame << main_ui_->columnEditorFrame
            << main_ui_->preferenceEditorFrame << main_ui_->filterExpressionFrame;

    frame_list.removeAll(show_frame);
    foreach(AccordionFrame *af, frame_list) af->animatedHide();

    if (toggle) {
        if (show_frame->isVisible()) {
            show_frame->animatedHide();
            return;
        }
    }
    show_frame->animatedShow();
}

void WiresharkMainWindow::showColumnEditor(int column)
{
    previous_focus_ = mainApp->focusWidget();
    connect(previous_focus_, SIGNAL(destroyed()), this, SLOT(resetPreviousFocus()));
    main_ui_->columnEditorFrame->editColumn(column);
    showAccordionFrame(main_ui_->columnEditorFrame);
}

void WiresharkMainWindow::showPreferenceEditor()
{
    showAccordionFrame(main_ui_->preferenceEditorFrame);
}

void WiresharkMainWindow::initViewColorizeMenu()
{
    QList<QAction *> cc_actions = QList<QAction *>()
            << main_ui_->actionViewColorizeConversation1 << main_ui_->actionViewColorizeConversation2
            << main_ui_->actionViewColorizeConversation3 << main_ui_->actionViewColorizeConversation4
            << main_ui_->actionViewColorizeConversation5 << main_ui_->actionViewColorizeConversation6
            << main_ui_->actionViewColorizeConversation7 << main_ui_->actionViewColorizeConversation8
            << main_ui_->actionViewColorizeConversation9 << main_ui_->actionViewColorizeConversation10;

    guint8 color_num = 1;

    foreach(QAction *cc_action, cc_actions) {
        cc_action->setData(color_num);
        connect(cc_action, SIGNAL(triggered()), this, SLOT(colorizeConversation()));

        const color_filter_t *colorf = color_filters_tmp_color(color_num);
        if (colorf) {
            QColor bg = ColorUtils::fromColorT(colorf->bg_color);
            QColor fg = ColorUtils::fromColorT(colorf->fg_color);
            cc_action->setIcon(StockIcon::colorIcon(bg.rgb(), fg.rgb(), QString::number(color_num)));
        }
        color_num++;
    }

#ifdef Q_OS_MAC
    // Spotlight uses Cmd+Space
    main_ui_->actionViewColorizeResetColorization->setShortcut(QKeySequence("Meta+Space"));
#endif
}

void WiresharkMainWindow::addStatsPluginsToMenu() {
    GList          *cfg_list = stats_tree_get_cfg_list();
    QAction        *stats_tree_action;
    QMenu          *parent_menu;
    bool            first_item = true;

    for (GList *iter = g_list_first(cfg_list); iter; iter = gxx_list_next(iter)) {
        stats_tree_cfg *cfg = gxx_list_data(stats_tree_cfg *, iter);
        if (!menu_groups_.contains(cfg->stat_group)) {
            continue;
        }
        if (cfg->plugin) {
            if (first_item) {
                main_ui_->menuStatistics->addSeparator();
                first_item = false;
            }

            parent_menu = main_ui_->menuStatistics;
            // gtk/main_menubar.c compresses double slashes, hence SkipEmptyParts
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
            QStringList cfg_name_parts = QString(cfg->name).split("/", Qt::SkipEmptyParts);
#else
            QStringList cfg_name_parts = QString(cfg->name).split("/", QString::SkipEmptyParts);
#endif
            if (cfg_name_parts.isEmpty()) continue;

            QString stat_name = cfg_name_parts.takeLast();
            if (!cfg_name_parts.isEmpty()) {
                QString menu_name = cfg_name_parts.join("/");
                parent_menu = findOrAddMenu(parent_menu, menu_name);
            }

            stats_tree_action = new QAction(stat_name, this);
            stats_tree_action->setData(QString::fromUtf8(cfg->abbr));
            parent_menu->addAction(stats_tree_action);
            connect(stats_tree_action, &QAction::triggered, this, [this]() {
                QAction* action = qobject_cast<QAction*>(sender());
                if (action) {
                    openStatisticsTreeDialog(action->data().toString().toUtf8());
                }
            });
        }
    }
    g_list_free(cfg_list);
}

void WiresharkMainWindow::setFeaturesEnabled(bool enabled)
{
    main_ui_->menuBar->setEnabled(enabled);
    main_ui_->mainToolBar->setEnabled(enabled);
    main_ui_->displayFilterToolBar->setEnabled(enabled);
    if (enabled)
    {
        main_ui_->statusBar->clearMessage();
#ifdef HAVE_LIBPCAP
        main_ui_->actionGoAutoScroll->setChecked(auto_scroll_live);
#endif
    }
    else
    {
        main_ui_->statusBar->showMessage(tr("Please wait while Wireshark is initializing…"));
    }
}

// Display Filter Toolbar

void WiresharkMainWindow::on_actionDisplayFilterExpression_triggered()
{
    DisplayFilterExpressionDialog *dfe_dialog = new DisplayFilterExpressionDialog(this);

    connect(dfe_dialog, SIGNAL(insertDisplayFilter(QString)),
            df_combo_box_->lineEdit(), SLOT(insertFilter(const QString &)));

    dfe_dialog->show();
}

void WiresharkMainWindow::on_actionNewDisplayFilterExpression_triggered()
{
    main_ui_->filterExpressionFrame->addExpression(df_combo_box_->lineEdit()->text());
}

void WiresharkMainWindow::onFilterSelected(QString filterText, bool prepare)
{
    if (filterText.length() <= 0)
        return;

    df_combo_box_->setDisplayFilter(filterText);
    // Holding down the Shift key will only prepare filter.
    if (!prepare)
        df_combo_box_->applyDisplayFilter();
}

void WiresharkMainWindow::onFilterPreferences()
{
    emit showPreferencesDialog(PrefsModel::typeToString(PrefsModel::FilterButtons));
}

void WiresharkMainWindow::onFilterEdit(int uatIndex)
{
    main_ui_->filterExpressionFrame->editExpression(uatIndex);
}

void WiresharkMainWindow::openStatCommandDialog(const QString &menu_path, const char *arg, void *userdata)
{
    QString slot = QString("statCommand%1").arg(menu_path);
    QMetaObject::invokeMethod(this, slot.toLatin1().constData(), Q_ARG(const char *, arg), Q_ARG(void *, userdata));
}

void WiresharkMainWindow::openTapParameterDialog(const QString cfg_str, const QString arg, void *userdata)
{
    TapParameterDialog *tp_dialog = TapParameterDialog::showTapParameterStatistics(*this, capture_file_, cfg_str, arg, userdata);
    if (!tp_dialog) return;

    connect(tp_dialog, SIGNAL(filterAction(QString, FilterAction::Action, FilterAction::ActionType)),
            this, SIGNAL(filterAction(QString, FilterAction::Action, FilterAction::ActionType)));
    connect(tp_dialog, SIGNAL(updateFilter(QString)),
            df_combo_box_->lineEdit(), SLOT(setText(QString)));
    tp_dialog->show();
}

void WiresharkMainWindow::openTapParameterDialog()
{
    QAction *tpa = qobject_cast<QAction *>(QObject::sender());
    if (!tpa) return;

    const QString cfg_str = tpa->data().toString();
    openTapParameterDialog(cfg_str, NULL, NULL);
}

#if defined(HAVE_SOFTWARE_UPDATE) && defined(Q_OS_WIN)
void WiresharkMainWindow::softwareUpdateRequested() {
    // We could call testCaptureFileClose here, but that would give us yet
    // another dialog. Just try again later.
    if (capture_file_.capFile() && capture_file_.capFile()->state != FILE_CLOSED) {
        mainApp->rejectSoftwareUpdate();
    }
}
#endif

// File Menu

void WiresharkMainWindow::connectFileMenuActions()
{
    connect(main_ui_->actionFileOpen, &QAction::triggered, this,
            [this]() { openCaptureFile(); });

    connect(main_ui_->actionFileMerge, &QAction::triggered, this,
            [this]() { mergeCaptureFile(); });

    connect(main_ui_->actionFileImportFromHexDump, &QAction::triggered, this,
            [this]() { importCaptureFile(); });

    connect(main_ui_->actionFileClose, &QAction::triggered, this, [this]() {
        QString before_what(tr(" before closing the file"));
        if (testCaptureFileClose(before_what)) {
            showWelcome();
        }
    });

    connect(main_ui_->actionFileSave, &QAction::triggered, this,
        [this]() { saveCaptureFile(capture_file_.capFile(), false); });

    connect(main_ui_->actionFileSaveAs, &QAction::triggered, this,
        [this]() { saveAsCaptureFile(capture_file_.capFile()); });

    connect(main_ui_->actionFileSetListFiles, &QAction::triggered, this,
        [this]() { file_set_dialog_->show(); });

    connect(main_ui_->actionFileSetNextFile, &QAction::triggered, this, [this]() {
        fileset_entry *entry = fileset_get_next();

        if (entry) {
            QString new_cf_path = entry->fullname;
            openCaptureFile(new_cf_path);
        }
    });

    connect(main_ui_->actionFileSetPreviousFile, &QAction::triggered, this, [this]() {
        fileset_entry *entry = fileset_get_previous();

        if (entry) {
            QString new_cf_path = entry->fullname;
            openCaptureFile(new_cf_path);
        }
    });

    connect(main_ui_->actionFileExportPackets, &QAction::triggered, this,
        [this]() { exportSelectedPackets(); });

    connect(main_ui_->actionFileExportAsPlainText, &QAction::triggered, this,
        [this]() { exportDissections(export_type_text); });

    connect(main_ui_->actionFileExportAsCSV, &QAction::triggered, this,
        [this]() { exportDissections(export_type_csv); });

    connect(main_ui_->actionFileExportAsCArrays, &QAction::triggered, this,
        [this]() { exportDissections(export_type_carrays); });

    connect(main_ui_->actionFileExportAsPSML, &QAction::triggered, this,
        [this]() { exportDissections(export_type_psml); });

    connect(main_ui_->actionFileExportAsPDML, &QAction::triggered, this,
        [this]() { exportDissections(export_type_pdml); });

    connect(main_ui_->actionFileExportAsJSON, &QAction::triggered, this,
        [this]() { exportDissections(export_type_json); });

    connect(main_ui_->actionFileExportPacketBytes, &QAction::triggered, this,
        [this]() { exportPacketBytes(); });

    connect(main_ui_->actionFileExportPDU, &QAction::triggered, this,
        [this]() { exportPDU(); });

    connect(main_ui_->actionFileStripHeaders, &QAction::triggered, this,
        [this]() { stripPacketHeaders(); });

    connect(main_ui_->actionFileExportTLSSessionKeys, &QAction::triggered, this,
        [this]() { exportTLSSessionKeys(); });

    connect(main_ui_->actionFilePrint, &QAction::triggered, this,
        [this]() { printFile(); });
}

void WiresharkMainWindow::exportPacketBytes()
{
    QString file_name;

    if (!capture_file_.capFile() || !capture_file_.capFile()->finfo_selected) return;

    file_name = WiresharkFileDialog::getSaveFileName(this,
                                            mainApp->windowTitleString(tr("Export Selected Packet Bytes")),
                                            mainApp->lastOpenDir().canonicalPath(),
                                            tr("Raw data (*.bin *.dat *.raw);;All Files (" ALL_FILES_WILDCARD ")")
                                            );

    if (file_name.length() > 0) {
        const guint8 *data_p;

        data_p = tvb_get_ptr(capture_file_.capFile()->finfo_selected->ds_tvb, 0, -1) +
                capture_file_.capFile()->finfo_selected->start;
        write_file_binary_mode(qUtf8Printable(file_name), data_p, capture_file_.capFile()->finfo_selected->length);

        /* Save the directory name for future file dialogs. */
        mainApp->setLastOpenDirFromFilename(file_name);
    }
}

void WiresharkMainWindow::exportPDU()
{
    ExportPDUDialog *exportpdu_dialog = new ExportPDUDialog(this);

    if (exportpdu_dialog->isMinimized() == true)
    {
        exportpdu_dialog->showNormal();
    }
    else
    {
        exportpdu_dialog->show();
    }

    exportpdu_dialog->raise();
    exportpdu_dialog->activateWindow();
}

void WiresharkMainWindow::stripPacketHeaders()
{
    StripHeadersDialog *stripheaders_dialog = new StripHeadersDialog(this);

    if (stripheaders_dialog->isMinimized() == true)
    {
        stripheaders_dialog->showNormal();
    }
    else
    {
        stripheaders_dialog->show();
    }

    stripheaders_dialog->raise();
    stripheaders_dialog->activateWindow();
}


void WiresharkMainWindow::exportTLSSessionKeys()
{
    QString file_name;
    QString save_title;
    int keylist_len;

    keylist_len = ssl_session_key_count();
    /* don't show up the dialog, if no data has to be saved */
    if (keylist_len < 1) {
        /* shouldn't happen as the menu item should have been greyed out */
        QMessageBox::warning(
                    this,
                    tr("No Keys"),
                    tr("There are no TLS Session Keys to save."),
                    QMessageBox::Ok
                    );
        return;
    }

    save_title.append(mainApp->windowTitleString(tr("Export TLS Session Keys (%Ln key(s))", "", keylist_len)));
    file_name = WiresharkFileDialog::getSaveFileName(this,
                                            save_title,
                                            mainApp->lastOpenDir().canonicalPath(),
                                            tr("TLS Session Keys (*.keys *.txt);;All Files (" ALL_FILES_WILDCARD ")")
                                            );
    if (file_name.length() > 0) {
        gsize keylist_length;
        gchar *keylist = ssl_export_sessions(&keylist_length);
        write_file_binary_mode(qUtf8Printable(file_name), keylist, keylist_length);

        /* Save the directory name for future file dialogs. */
        mainApp->setLastOpenDirFromFilename(file_name);
        g_free(keylist);
    }
}

void WiresharkMainWindow::printFile()
{
    capture_file *cf = capture_file_.capFile();
    g_return_if_fail(cf);

    QList<int> rows = packet_list_->selectedRows(true);

    QStringList entries;
    foreach (int row, rows)
        entries << QString::number(row);
    QString selRange = entries.join(",");

    PrintDialog * pdlg_ = new PrintDialog(this, cf, selRange);
    pdlg_->setWindowModality(Qt::ApplicationModal);
    pdlg_->show();
}

// Edit Menu

void WiresharkMainWindow::connectEditMenuActions()
{
    connect(main_ui_->actionCopyAllVisibleItems, &QAction::triggered, this,
            [this]() { copySelectedItems(CopyAllVisibleItems); });

    connect(main_ui_->actionCopyListAsText, &QAction::triggered, this,
            [this]() { copySelectedItems(CopyListAsText); });

    connect(main_ui_->actionCopyListAsCSV, &QAction::triggered, this,
            [this]() { copySelectedItems(CopyListAsCSV); });

    connect(main_ui_->actionCopyListAsYAML, &QAction::triggered, this,
            [this]() { copySelectedItems(CopyListAsYAML); });

    connect(main_ui_->actionCopyAllVisibleSelectedTreeItems, &QAction::triggered, this,
            [this]() { copySelectedItems(CopyAllVisibleSelectedTreeItems); });

    connect(main_ui_->actionEditCopyDescription, &QAction::triggered, this,
            [this]() { copySelectedItems(CopySelectedDescription); });

    connect(main_ui_->actionEditCopyFieldName, &QAction::triggered, this,
            [this]() { copySelectedItems(CopySelectedFieldName); });

    connect(main_ui_->actionEditCopyValue, &QAction::triggered, this,
            [this]() { copySelectedItems(CopySelectedValue); });

    connect(main_ui_->actionEditCopyAsFilter, &QAction::triggered, this,
            [this]() { matchFieldFilter(FilterAction::ActionCopy, FilterAction::ActionTypePlain); });

    connect(main_ui_->actionEditFindPacket, &QAction::triggered, this,
            [this]() { findPacket(); });

    connect(main_ui_->actionEditFindNext, &QAction::triggered, this,
            [this]() { main_ui_->searchFrame->findNext(); });

    connect(main_ui_->actionEditFindPrevious, &QAction::triggered, this,
            [this]() { main_ui_->searchFrame->findPrevious(); });

    // The items below are used in the packet list and detail context menus.
    // Use QueuedConnections so that the context menus aren't destroyed
    // prematurely.
    connect(main_ui_->actionEditMarkPacket, &QAction::triggered, this, [this]() {
        freeze();
        packet_list_->markFrame();
        thaw();
        setMenusForSelectedPacket();
    }, Qt::QueuedConnection);

    connect(main_ui_->actionEditMarkAllDisplayed, &QAction::triggered, this, [this]() {
        freeze();
        packet_list_->markAllDisplayedFrames(true);
        thaw();
        setMenusForSelectedPacket();
    }, Qt::QueuedConnection);

    connect(main_ui_->actionEditUnmarkAllDisplayed, &QAction::triggered, this, [this]() {
        freeze();
        packet_list_->markAllDisplayedFrames(false);
        thaw();
        setMenusForSelectedPacket();
    }, Qt::QueuedConnection);

    connect(main_ui_->actionEditNextMark, &QAction::triggered, this, [this]() {
        if (capture_file_.capFile()) {
            cf_find_packet_marked(capture_file_.capFile(), SD_FORWARD);
        }
    }, Qt::QueuedConnection);

    connect(main_ui_->actionEditPreviousMark, &QAction::triggered, this, [this]() {
        if (capture_file_.capFile()) {
            cf_find_packet_marked(capture_file_.capFile(), SD_BACKWARD);
        }
    }, Qt::QueuedConnection);

    connect(main_ui_->actionEditIgnorePacket, &QAction::triggered, this, [this]() {
        freeze();
        packet_list_->ignoreFrame();
        thaw();
        setMenusForSelectedPacket();
    }, Qt::QueuedConnection);

    connect(main_ui_->actionEditIgnoreAllDisplayed, &QAction::triggered, this, [this]() {
        freeze();
        packet_list_->ignoreAllDisplayedFrames(true);
        thaw();
        setMenusForSelectedPacket();
    }, Qt::QueuedConnection);

    connect(main_ui_->actionEditUnignoreAllDisplayed, &QAction::triggered, this, [this]() {
        freeze();
        packet_list_->ignoreAllDisplayedFrames(false);
        thaw();
        setMenusForSelectedPacket();
    }, Qt::QueuedConnection);

    connect(main_ui_->actionEditSetTimeReference, &QAction::triggered, this, [this]() {
        packet_list_->setTimeReference();
        setMenusForSelectedPacket();
    }, Qt::QueuedConnection);

    connect(main_ui_->actionEditUnsetAllTimeReferences, &QAction::triggered, this, [this]() {
        packet_list_->unsetAllTimeReferences();
        setMenusForSelectedPacket();
    }, Qt::QueuedConnection);

    connect(main_ui_->actionEditNextTimeReference, &QAction::triggered, this, [this]() {
        if (!capture_file_.capFile()) return;
        cf_find_packet_time_reference(capture_file_.capFile(), SD_FORWARD);
    }, Qt::QueuedConnection);

    connect(main_ui_->actionEditPreviousTimeReference, &QAction::triggered, this, [this]() {
        if (!capture_file_.capFile()) return;
        cf_find_packet_time_reference(capture_file_.capFile(), SD_BACKWARD);
    }, Qt::QueuedConnection);

    connect(main_ui_->actionEditTimeShift, &QAction::triggered, this,
            [this]() { editTimeShift(); }, Qt::QueuedConnection);

    connect(main_ui_->actionDeleteAllPacketComments, &QAction::triggered, this,
            [this]() { deleteAllPacketComments(); }, Qt::QueuedConnection);

    connect(main_ui_->actionEditConfigurationProfiles, &QAction::triggered, this,
            [this]() { editConfigurationProfiles(); }, Qt::QueuedConnection);

    connect(main_ui_->actionEditPreferences, &QAction::triggered, this,
            [this]() { showPreferencesDialog(PrefsModel::typeToString(PrefsModel::Appearance)); }, Qt::QueuedConnection);
}

// XXX This should probably be somewhere else.
void WiresharkMainWindow::copySelectedItems(WiresharkMainWindow::CopySelected selection_type)
{
    char label_str[ITEM_LABEL_LENGTH];
    QString clip;

    if (!capture_file_.capFile()) return;

    field_info *finfo_selected = capture_file_.capFile()->finfo_selected;

    switch (selection_type) {
    case CopySelectedDescription:
        if (proto_tree_->selectionModel()->hasSelection()) {
            QModelIndex idx = proto_tree_->selectionModel()->selectedIndexes().first();
            clip = idx.data(Qt::DisplayRole).toString();
        }
        break;
    case CopySelectedFieldName:
        if (finfo_selected && finfo_selected->hfinfo->abbrev != 0) {
            clip.append(finfo_selected->hfinfo->abbrev);
        }
        break;
    case CopySelectedValue:
        if (finfo_selected && capture_file_.capFile()->edt != 0) {
            gchar* field_str = get_node_field_value(finfo_selected, capture_file_.capFile()->edt);
            clip.append(field_str);
            g_free(field_str);
        }
        break;
    case CopyAllVisibleItems:
        clip = proto_tree_->toString();
        break;
    case CopyAllVisibleSelectedTreeItems:
        if (proto_tree_->selectionModel()->hasSelection()) {
            clip = proto_tree_->toString(proto_tree_->selectionModel()->selectedIndexes().first());
        }
        break;
    case CopyListAsText:
    case CopyListAsCSV:
    case CopyListAsYAML:
        if (packet_list_->selectedRows().count() > 0)
        {
            QList<int> rows = packet_list_->selectedRows();
            QStringList content;

            PacketList::SummaryCopyType copyType = PacketList::CopyAsText;
            if (selection_type == CopyListAsCSV)
                copyType = PacketList::CopyAsCSV;
            else if (selection_type == CopyListAsYAML)
                copyType = PacketList::CopyAsYAML;

            if ((copyType == PacketList::CopyAsText) ||
                (copyType == PacketList::CopyAsCSV)) {
                QString headerEntry = packet_list_->createHeaderSummaryText(copyType);
                content << headerEntry;
            }
            foreach (int row, rows)
            {
                QModelIndex idx = packet_list_->model()->index(row, 0);
                if (! idx.isValid())
                    continue;

                QString entry = packet_list_->createSummaryText(idx, copyType);
                content << entry;
            }

            if (content.count() > 0) {
                clip = content.join("\n");
                //
                // Each YAML item ends with a newline, so the string
                // ends with a newline already if it's CopyListAsYAML.
                // If we add a newline, there'd be an extra blank
                // line.
                //
                // Otherwise, we've used newlines as separators, not
                // terminators, so there's no final newline.  Add it.
                //
                if (selection_type != CopyListAsYAML)
                    clip += "\n";
            }
        }
        break;
    }

    if (clip.length() == 0) {
        /* If no representation then... Try to read the value */
        proto_item_fill_label(capture_file_.capFile()->finfo_selected, label_str);
        clip.append(label_str);
    }

    if (clip.length()) {
        mainApp->clipboard()->setText(clip);
    } else {
        QString err = tr("Couldn't copy text. Try another item.");
        mainApp->pushStatus(WiresharkApplication::TemporaryStatus, err);
    }
}

void WiresharkMainWindow::findPacket()
{
    if (! packet_list_->model() || packet_list_->model()->rowCount() < 1) {
        return;
    }
    previous_focus_ = mainApp->focusWidget();
    connect(previous_focus_, SIGNAL(destroyed()), this, SLOT(resetPreviousFocus()));
    if (!main_ui_->searchFrame->isVisible()) {
        showAccordionFrame(main_ui_->searchFrame, true);
    } else {
        main_ui_->searchFrame->animatedHide();
    }
    main_ui_->searchFrame->setFocus();
}

void WiresharkMainWindow::editTimeShift()
{
    TimeShiftDialog *ts_dialog = new TimeShiftDialog(this, capture_file_.capFile());
    connect(ts_dialog, SIGNAL(finished(int)), this, SLOT(editTimeShiftFinished(int)));

    connect(this, SIGNAL(setCaptureFile(capture_file*)),
            ts_dialog, SLOT(setCaptureFile(capture_file*)));
    connect(ts_dialog, SIGNAL(timeShifted()), packet_list_, SLOT(applyTimeShift()));

    ts_dialog->setWindowModality(Qt::ApplicationModal);
    ts_dialog->setAttribute(Qt::WA_DeleteOnClose);
    ts_dialog->show();
}

void WiresharkMainWindow::editTimeShiftFinished(int)
{
    if (capture_file_.capFile()->unsaved_changes) {
        updateForUnsavedChanges();
    }
}

void WiresharkMainWindow::addPacketComment()
{
    QList<int> rows = selectedRows();
    if (rows.count() == 0)
        return;

    frame_data * fdata = frameDataForRow(rows.at(0));
    if (! fdata)
        return;

    PacketCommentDialog* pc_dialog;
    pc_dialog = new PacketCommentDialog(false, this, NULL);
    connect(pc_dialog, &QDialog::finished, std::bind(&WiresharkMainWindow::addPacketCommentFinished, this, pc_dialog, std::placeholders::_1));
    pc_dialog->setWindowModality(Qt::ApplicationModal);
    pc_dialog->setAttribute(Qt::WA_DeleteOnClose);
    pc_dialog->show();
}

void WiresharkMainWindow::addPacketCommentFinished(PacketCommentDialog* pc_dialog _U_, int result _U_)
{
    if (result == QDialog::Accepted) {
        packet_list_->addPacketComment(pc_dialog->text());
        updateForUnsavedChanges();
    }
}

void WiresharkMainWindow::editPacketComment()
{
    QList<int> rows = selectedRows();
    if (rows.count() != 1)
        return;

    QAction *ra = qobject_cast<QAction*>(sender());
    guint nComment = ra->data().toUInt();
    PacketCommentDialog* pc_dialog;
    pc_dialog = new PacketCommentDialog(true, this, packet_list_->getPacketComment(nComment));
    connect(pc_dialog, &QDialog::finished, std::bind(&WiresharkMainWindow::editPacketCommentFinished, this, pc_dialog, std::placeholders::_1, nComment));
    pc_dialog->setWindowModality(Qt::ApplicationModal);
    pc_dialog->setAttribute(Qt::WA_DeleteOnClose);
    pc_dialog->show();
}

void WiresharkMainWindow::editPacketCommentFinished(PacketCommentDialog* pc_dialog _U_, int result _U_, guint nComment)
{
    if (result == QDialog::Accepted) {
        packet_list_->setPacketComment(nComment, pc_dialog->text());
        updateForUnsavedChanges();
    }
}

void WiresharkMainWindow::deletePacketComment()
{
    QAction *ra = qobject_cast<QAction*>(sender());
    guint nComment = ra->data().toUInt();
    packet_list_->setPacketComment(nComment, QString(""));
    updateForUnsavedChanges();
}

void WiresharkMainWindow::deleteCommentsFromPackets()
{
    packet_list_->deleteCommentsFromPackets();
    updateForUnsavedChanges();
}

void WiresharkMainWindow::deleteAllPacketComments()
{
    QMessageBox *msg_dialog = new QMessageBox();
    connect(msg_dialog, SIGNAL(finished(int)), this, SLOT(deleteAllPacketCommentsFinished(int)));

    msg_dialog->setIcon(QMessageBox::Question);
    msg_dialog->setText(tr("Are you sure you want to remove all packet comments?"));

    msg_dialog->setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
    msg_dialog->setDefaultButton(QMessageBox::Ok);

    msg_dialog->setWindowModality(Qt::ApplicationModal);
    msg_dialog->setAttribute(Qt::WA_DeleteOnClose);
    msg_dialog->show();
}

void WiresharkMainWindow::deleteAllPacketCommentsFinished(int result)
{
    if (result == QMessageBox::Ok) {
        /* XXX Do we need a wait/hourglass for large files? */
        packet_list_->deleteAllPacketComments();
        updateForUnsavedChanges();
    }
}

void WiresharkMainWindow::editConfigurationProfiles()
{
    ProfileDialog *cp_dialog = new ProfileDialog();
    cp_dialog->setWindowModality(Qt::ApplicationModal);
    cp_dialog->setAttribute(Qt::WA_DeleteOnClose);
    cp_dialog->show();
}

void WiresharkMainWindow::showPreferencesDialog(QString module_name)
{
    PreferencesDialog *pref_dialog = new PreferencesDialog(this);
    connect(pref_dialog, SIGNAL(destroyed(QObject*)), mainApp, SLOT(flushAppSignals()));
    saveWindowGeometry();  // Save in case the layout panes are rearranged

    pref_dialog->setPane(module_name);
    pref_dialog->setWindowModality(Qt::ApplicationModal);
    pref_dialog->setAttribute(Qt::WA_DeleteOnClose);
    pref_dialog->show();
}

// View Menu

void WiresharkMainWindow::connectViewMenuActions()
{
    connect(main_ui_->actionViewFullScreen, &QAction::triggered, this, [this](bool checked) {
        if (checked) {
            // Save the state for future restore
            was_maximized_ = this->isMaximized();
            this->showFullScreen();
        } else {
            // Restore the previous state
            if (was_maximized_) {
                this->showMaximized();
            } else {
                this->showNormal();
            }
        }
    });

    connect(main_ui_->actionViewTimeDisplaySecondsWithHoursAndMinutes, &QAction::triggered, this,
            [this](bool checked) { setTimeDisplaySecondsWithHoursAndMinutes(checked); });

    connect(main_ui_->actionViewEditResolvedName, &QAction::triggered, this,
            [this]() { editResolvedName(); });

    connect(main_ui_->actionViewNameResolutionPhysical, &QAction::triggered, this,
            [this]() { setNameResolution(); });

    connect(main_ui_->actionViewNameResolutionNetwork, &QAction::triggered, this,
            [this]() { setNameResolution(); });

    connect(main_ui_->actionViewNameResolutionTransport, &QAction::triggered, this,
            [this]() { setNameResolution(); });

    connect(main_ui_->actionViewZoomIn, &QAction::triggered, this, [this]() {
        recent.gui_zoom_level++;
        zoomText();
    });

    connect(main_ui_->actionViewZoomOut, &QAction::triggered, this, [this]() {
        recent.gui_zoom_level--;
        zoomText();
    });

    connect(main_ui_->actionViewNormalSize, &QAction::triggered, this, [this]() {
        recent.gui_zoom_level = 0;
        zoomText();
    });

    connect(main_ui_->actionViewExpandSubtrees, &QAction::triggered,
            proto_tree_, &ProtoTree::expandSubtrees);

    connect(main_ui_->actionViewCollapseSubtrees, &QAction::triggered,
            proto_tree_, &ProtoTree::collapseSubtrees);

    connect(main_ui_->actionViewExpandAll, &QAction::triggered,
            proto_tree_, &ProtoTree::expandAll);

    connect(main_ui_->actionViewCollapseAll, &QAction::triggered,
            proto_tree_, &ProtoTree::collapseAll);

    connect(main_ui_->actionViewColorizePacketList, &QAction::triggered, this, [this](bool checked) {
        recent.packet_list_colorize = checked;
        packet_list_->recolorPackets();
    });

    connect(main_ui_->actionViewColoringRules, &QAction::triggered, this,
            [this]() { showColoringRulesDialog(); });

    connect(main_ui_->actionViewColorizeResetColorization, &QAction::triggered, this, [this]() {
        gchar *err_msg = NULL;
        if (!color_filters_reset_tmp(&err_msg)) {
            simple_dialog(ESD_TYPE_ERROR, ESD_BTN_OK, "%s", err_msg);
            g_free(err_msg);
        }
        packet_list_->recolorPackets();
        setMenusForSelectedPacket();
    });

    connect(main_ui_->actionViewColorizeNewColoringRule, &QAction::triggered, this,
            [this]() { colorizeConversation(true); });

    connect(main_ui_->actionViewResetLayout, &QAction::triggered, this, [this]() {
        recent.gui_geometry_main_upper_pane = 0;
        recent.gui_geometry_main_lower_pane = 0;
        applyRecentPaneGeometry();
    });

    connect(main_ui_->actionViewResizeColumns, &QAction::triggered, this, [this]() {
        if (! packet_list_->model())
            return;
        for (int col = 0; col < packet_list_->model()->columnCount(); col++) {
            packet_list_->resizeColumnToContents(col);
            recent_set_column_width(col, packet_list_->columnWidth(col));
        }
    });

    connect(main_ui_->actionViewInternalsConversationHashTables, &QAction::triggered, this, [this]() {
        ConversationHashTablesDialog *conversation_hash_tables_dlg = new ConversationHashTablesDialog(this);
        conversation_hash_tables_dlg->show();
    });

    connect(main_ui_->actionViewInternalsDissectorTables, &QAction::triggered, this, [this]() {
        DissectorTablesDialog *dissector_tables_dlg = new DissectorTablesDialog(this);
        dissector_tables_dlg->show();
    });

    connect(main_ui_->actionViewInternalsSupportedProtocols, &QAction::triggered, this, [this]() {
        SupportedProtocolsDialog *supported_protocols_dlg = new SupportedProtocolsDialog(this);
        supported_protocols_dlg->show();
    });

    connect(main_ui_->actionViewShowPacketInNewWindow, &QAction::triggered, this,
            [this]() { openPacketDialog(); });

    // This is only used in ProtoTree. Defining it here makes more sense.
    connect(main_ui_->actionContextShowLinkedPacketInNewWindow, &QAction::triggered, this,
            [this]() { openPacketDialog(true); });

    connect(main_ui_->actionViewReload_as_File_Format_or_Capture, &QAction::triggered, this,
            [this]() { reloadCaptureFileAsFormatOrCapture(); });

    connect(main_ui_->actionViewReload, &QAction::triggered, this,
            [this]() { reloadCaptureFile(); });
}

void WiresharkMainWindow::showHideMainWidgets(QAction *action)
{
    if (!action) {
        return;
    }
    bool show = action->isChecked();
    QWidget *widget = action->data().value<QWidget*>();

    // We may have come from the toolbar context menu, so check/uncheck each
    // action as well.
    if (widget == main_ui_->mainToolBar) {
        recent.main_toolbar_show = show;
        main_ui_->actionViewMainToolbar->setChecked(show);
    } else if (widget == main_ui_->displayFilterToolBar) {
        recent.filter_toolbar_show = show;
        main_ui_->actionViewFilterToolbar->setChecked(show);
#if defined(HAVE_LIBNL) && defined(HAVE_NL80211)
    } else if (widget == main_ui_->wirelessToolBar) {
        recent.wireless_toolbar_show = show;
        main_ui_->actionViewWirelessToolbar->setChecked(show);
#endif
    } else if (widget == main_ui_->statusBar) {
        recent.statusbar_show = show;
        main_ui_->actionViewStatusBar->setChecked(show);
    } else if (widget == packet_list_) {
        recent.packet_list_show = show;
        main_ui_->actionViewPacketList->setChecked(show);
    } else if (widget == proto_tree_) {
        recent.tree_view_show = show;
        main_ui_->actionViewPacketDetails->setChecked(show);
    } else if (widget == byte_view_tab_) {
        recent.byte_view_show = show;
        main_ui_->actionViewPacketBytes->setChecked(show);
    } else if (widget == packet_diagram_) {
        recent.packet_diagram_show = show;
        main_ui_->actionViewPacketDiagram->setChecked(show);
    } else {
        foreach(QAction *action, main_ui_->menuInterfaceToolbars->actions()) {
            QToolBar *toolbar = action->data().value<QToolBar *>();
            if (widget == toolbar) {
                GList *entry = g_list_find_custom(recent.interface_toolbars, action->text().toUtf8(), (GCompareFunc)strcmp);
                if (show && !entry) {
                    recent.interface_toolbars = g_list_append(recent.interface_toolbars, g_strdup(action->text().toUtf8()));
                } else if (!show && entry) {
                    recent.interface_toolbars = g_list_remove(recent.interface_toolbars, entry->data);
                }
                action->setChecked(show);
            }
        }

        ext_toolbar_t * toolbar = VariantPointer<ext_toolbar_t>::asPtr(action->data());
        if (toolbar) {
            GList *entry = g_list_find_custom(recent.gui_additional_toolbars, toolbar->name, (GCompareFunc)strcmp);
            if (show && !entry) {
                recent.gui_additional_toolbars = g_list_append(recent.gui_additional_toolbars, g_strdup(toolbar->name));
            } else if (!show && entry) {
                recent.gui_additional_toolbars = g_list_remove(recent.gui_additional_toolbars, entry->data);
            }
            action->setChecked(show);

            QList<QToolBar *> toolbars = findChildren<QToolBar *>();
            foreach(QToolBar *bar, toolbars) {
                AdditionalToolBar *iftoolbar = dynamic_cast<AdditionalToolBar *>(bar);
                if (iftoolbar && iftoolbar->menuName().compare(toolbar->name) == 0) {
                    iftoolbar->setVisible(show);
                }
            }
        }
    }

    if (widget) {
        widget->setVisible(show);
    }
}

void WiresharkMainWindow::setTimestampFormat(QAction *action)
{
    if (!action) {
        return;
    }
    ts_type tsf = action->data().value<ts_type>();
    if (recent.gui_time_format != tsf) {
        timestamp_set_type(tsf);
        recent.gui_time_format = tsf;

        if (packet_list_) {
            packet_list_->resetColumns();
            packet_list_->resizeAllColumns(true);
        }
    }
}

void WiresharkMainWindow::setTimestampPrecision(QAction *action)
{
    if (!action) {
        return;
    }
    ts_precision tsp = action->data().value<ts_precision>();
    if (recent.gui_time_precision != tsp) {
        timestamp_set_precision(tsp);
        recent.gui_time_precision = tsp;

        if (packet_list_) {
            packet_list_->resetColumns();
            packet_list_->resizeAllColumns(true);
        }
    }
}

void WiresharkMainWindow::setTimeDisplaySecondsWithHoursAndMinutes(bool checked)
{
    if (checked) {
        recent.gui_seconds_format = TS_SECONDS_HOUR_MIN_SEC;
    } else {
        recent.gui_seconds_format = TS_SECONDS_DEFAULT;
    }
    timestamp_set_seconds_type(recent.gui_seconds_format);

    if (packet_list_) {
        packet_list_->resetColumns();
        packet_list_->resizeAllColumns(true);
    }
}

void WiresharkMainWindow::editResolvedName()
{
    //int column = packet_list_->selectedColumn();
    int column = -1;

    if (packet_list_->currentIndex().isValid()) {
        column = packet_list_->currentIndex().column();
    }

    main_ui_->addressEditorFrame->editAddresses(capture_file_, column);
    showAccordionFrame(main_ui_->addressEditorFrame);
}

void WiresharkMainWindow::setNameResolution()
{
    gbl_resolv_flags.mac_name = main_ui_->actionViewNameResolutionPhysical->isChecked() ? TRUE : FALSE;
    gbl_resolv_flags.network_name = main_ui_->actionViewNameResolutionNetwork->isChecked() ? TRUE : FALSE;
    gbl_resolv_flags.transport_name = main_ui_->actionViewNameResolutionTransport->isChecked() ? TRUE : FALSE;

    if (packet_list_) {
        packet_list_->resetColumns();
    }
    mainApp->emitAppSignal(WiresharkApplication::NameResolutionChanged);
}

void WiresharkMainWindow::zoomText()
{
    mainApp->zoomTextFont(recent.gui_zoom_level);
}

void WiresharkMainWindow::showColoringRulesDialog()
{
    ColoringRulesDialog *coloring_rules_dialog = new ColoringRulesDialog(this);
    connect(coloring_rules_dialog, &ColoringRulesDialog::accepted,
            packet_list_, &PacketList::recolorPackets);
    connect(coloring_rules_dialog, &ColoringRulesDialog::filterAction,
            this, &WiresharkMainWindow::filterAction);

    coloring_rules_dialog->setWindowModality(Qt::ApplicationModal);
    coloring_rules_dialog->setAttribute(Qt::WA_DeleteOnClose);
    coloring_rules_dialog->show();
}

// actionViewColorizeConversation1 - 10
void WiresharkMainWindow::colorizeConversation(bool create_rule)
{
    QAction *colorize_action = qobject_cast<QAction *>(sender());
    if (!colorize_action) return;

    if (capture_file_.capFile() && selectedRows().count() > 0) {
        packet_info *pi = capture_file_.packetInfo();
        guint8 cc_num = colorize_action->data().toUInt();
        gchar *filter = conversation_filter_from_packet(pi);
        if (filter == NULL) {
            mainApp->pushStatus(WiresharkApplication::TemporaryStatus, tr("Unable to build conversation filter."));
            return;
        }

        if (create_rule) {
            ColoringRulesDialog coloring_rules_dialog(this, filter);
            connect(&coloring_rules_dialog, &ColoringRulesDialog::accepted,
                packet_list_, &PacketList::recolorPackets);
            connect(&coloring_rules_dialog, &ColoringRulesDialog::filterAction,
                this, &WiresharkMainWindow::filterAction);
            coloring_rules_dialog.exec();
        } else {
            gchar *err_msg = NULL;
            if (!color_filters_set_tmp(cc_num, filter, FALSE, &err_msg)) {
                simple_dialog(ESD_TYPE_ERROR, ESD_BTN_OK, "%s", err_msg);
                g_free(err_msg);
            }
            packet_list_->recolorPackets();
        }
    }
    setMenusForSelectedPacket();
}

void WiresharkMainWindow::colorizeActionTriggered()
{
    QByteArray filter;
    int color_number = -1;

    ConversationAction *conv_action = qobject_cast<ConversationAction *>(sender());
    if (conv_action) {
        filter = conv_action->filter();
        color_number = conv_action->colorNumber();
    } else {
        ColorizeAction *colorize_action = qobject_cast<ColorizeAction *>(sender());
        if (colorize_action) {
            filter = colorize_action->filter();
            color_number = colorize_action->colorNumber();
        }
    }

    colorizeWithFilter(filter, color_number);
}

void WiresharkMainWindow::colorizeWithFilter(QByteArray filter, int color_number)
{
    if (filter.isEmpty()) return;

    if (color_number > 0) {
        // Assume "Color X"
        gchar *err_msg = NULL;
        if (!color_filters_set_tmp(color_number, filter.constData(), FALSE, &err_msg)) {
            simple_dialog(ESD_TYPE_ERROR, ESD_BTN_OK, "%s", err_msg);
            g_free(err_msg);
        }
        packet_list_->recolorPackets();
    } else {
        // New coloring rule
        ColoringRulesDialog coloring_rules_dialog(window(), filter);
        connect(&coloring_rules_dialog, &ColoringRulesDialog::accepted,
            packet_list_, &PacketList::recolorPackets);
        connect(&coloring_rules_dialog, &ColoringRulesDialog::filterAction,
            this, &WiresharkMainWindow::filterAction);
        coloring_rules_dialog.exec();
    }
    main_ui_->actionViewColorizeResetColorization->setEnabled(tmp_color_filters_used());
}

void WiresharkMainWindow::openPacketDialog(bool from_reference)
{
    frame_data * fdata = Q_NULLPTR;

    /* Find the frame for which we're popping up a dialog */
    if (from_reference) {
        guint32 framenum = fvalue_get_uinteger(&(capture_file_.capFile()->finfo_selected->value));
        if (framenum == 0)
            return;

        fdata = frame_data_sequence_find(capture_file_.capFile()->provider.frames, framenum);
    } else if (selectedRows().count() == 1) {
        fdata = frameDataForRow(selectedRows().at(0));
    } else if (selectedRows().count() > 1)
        return;

    /* If we have a frame, pop up the dialog */
    if (fdata) {
        PacketDialog *packet_dialog = new PacketDialog(*this, capture_file_, fdata);

        connect(packet_dialog, &PacketDialog::showProtocolPreferences,
                this, &WiresharkMainWindow::showPreferencesDialog);
        connect(packet_dialog, SIGNAL(editProtocolPreference(preference*, pref_module*)),
                main_ui_->preferenceEditorFrame, SLOT(editPreference(preference*, pref_module*)));

        connect(this, &WiresharkMainWindow::closePacketDialogs, packet_dialog, &PacketDialog::close);
        zoomText(); // Emits mainApp->zoomMonospaceFont(QFont)

        packet_dialog->show();
    }
}

void WiresharkMainWindow::reloadCaptureFileAsFormatOrCapture()
{
    capture_file *cf = CaptureFile::globalCapFile();

    if (cf->unsaved_changes) {
        QString before_what(tr(" before reloading the file"));
        if (!testCaptureFileClose(before_what, Reload))
            return;
    }

    if (cf->open_type == WTAP_TYPE_AUTO)
        cf->open_type = open_info_name_to_type("MIME Files Format");
    else /* TODO: This should be latest format chosen by user */
        cf->open_type = WTAP_TYPE_AUTO;

    cf_reload(cf);
}

void WiresharkMainWindow::reloadCaptureFile()
{
    capture_file *cf = CaptureFile::globalCapFile();

    if (cf->unsaved_changes) {
        QString before_what(tr(" before reloading the file"));
        if (!testCaptureFileClose(before_what, Reload))
            return;
    }

    cf_reload(cf);
}


// Expand / collapse slots in proto_tree

// Go Menu

void WiresharkMainWindow::connectGoMenuActions()
{
    connect(main_ui_->actionGoGoToPacket, &QAction::triggered, this, [this]() {
        if (! packet_list_->model() || packet_list_->model()->rowCount() < 1) {
            return;
        }
        previous_focus_ = mainApp->focusWidget();
        connect(previous_focus_, SIGNAL(destroyed()), this, SLOT(resetPreviousFocus()));

        showAccordionFrame(main_ui_->goToFrame, true);
        if (main_ui_->goToFrame->isVisible()) {
            main_ui_->goToLineEdit->clear();
            main_ui_->goToLineEdit->setFocus();
        }
    });

    connect(main_ui_->actionGoGoToLinkedPacket, &QAction::triggered, this, [this]() {
        QAction *gta = qobject_cast<QAction*>(sender());
        if (!gta) return;

        bool ok = false;
        int packet_num = gta->data().toInt(&ok);
        if (!ok) return;

        packet_list_->goToPacket(packet_num);
    });

    connect(main_ui_->actionGoNextPacket, &QAction::triggered,
            packet_list_, &PacketList::goNextPacket);

    connect(main_ui_->actionGoPreviousPacket, &QAction::triggered,
            packet_list_, &PacketList::goPreviousPacket);

    connect(main_ui_->actionGoFirstPacket, &QAction::triggered,
            packet_list_, &PacketList::goFirstPacket);

    connect(main_ui_->actionGoLastPacket, &QAction::triggered,
            packet_list_, &PacketList::goLastPacket);

    connect(main_ui_->actionGoNextConversationPacket, &QAction::triggered, this,
            [this]() { goToConversationFrame(true); });

    connect(main_ui_->actionGoPreviousConversationPacket, &QAction::triggered, this,
            [this]() { goToConversationFrame(false); });

    connect(main_ui_->actionGoNextHistoryPacket, &QAction::triggered,
            packet_list_, &PacketList::goNextHistoryPacket);

    connect(main_ui_->actionGoPreviousHistoryPacket, &QAction::triggered,
            packet_list_, &PacketList::goPreviousHistoryPacket);

    connect(main_ui_->actionGoAutoScroll, &QAction::triggered, this,
            [this](bool checked) { packet_list_->setVerticalAutoScroll(checked); });
}

void WiresharkMainWindow::goToConversationFrame(bool go_next) {
    gchar     *filter       = NULL;
    dfilter_t *dfcode       = NULL;
    gboolean   found_packet = FALSE;
    packet_info *pi = capture_file_.packetInfo();

    if (!pi) {
        // No packet was selected, or multiple packets were selected.
        return;
    }

    /* Try to build a conversation
     * filter in the order TCP, UDP, IP, Ethernet and apply the
     * coloring */
    filter = conversation_filter_from_packet(pi);
    if (filter == NULL) {
        mainApp->pushStatus(WiresharkApplication::TemporaryStatus, tr("Unable to build conversation filter."));
        g_free(filter);
        return;
    }

    if (!dfilter_compile(filter, &dfcode, NULL)) {
        /* The attempt failed; report an error. */
        mainApp->pushStatus(WiresharkApplication::TemporaryStatus, tr("Error compiling filter for this conversation."));
        g_free(filter);
        return;
    }

    found_packet = cf_find_packet_dfilter(capture_file_.capFile(), dfcode, go_next ? SD_FORWARD : SD_BACKWARD);

    if (!found_packet) {
        /* We didn't find a packet */
        mainApp->pushStatus(WiresharkApplication::TemporaryStatus, tr("No previous/next packet in conversation."));
    }

    dfilter_free(dfcode);
    g_free(filter);
}

// Capture Menu

void WiresharkMainWindow::connectCaptureMenuActions()
{
#ifdef HAVE_LIBPCAP
    connect(main_ui_->actionCaptureOptions, &QAction::triggered, this,
            [this]() { showCaptureOptionsDialog(); });
#endif

    connect(main_ui_->actionCaptureStart, &QAction::triggered, this,
            [this]() { startCaptureTriggered(); });

    connect(main_ui_->actionCaptureStop, &QAction::triggered, this,
            [this]() { stopCapture(); });

    connect(main_ui_->actionCaptureRestart, &QAction::triggered, this, [this]() {
#ifdef HAVE_LIBPCAP
        QString before_what(tr(" before restarting the capture"));
        cap_session_.capture_opts->restart = TRUE;
        if (!testCaptureFileClose(before_what, Restart)) {
            return;
        }
        startCapture(QStringList());
#endif // HAVE_LIBPCAP
    });

    connect(main_ui_->actionCaptureCaptureFilters, &QAction::triggered, this, [this]() {
        if (!capture_filter_dlg_) {
            capture_filter_dlg_ = new FilterDialog(this, FilterDialog::CaptureFilter);
        }
        capture_filter_dlg_->show();
        capture_filter_dlg_->raise();
        capture_filter_dlg_->activateWindow();
    });

#ifdef HAVE_LIBPCAP
    connect(main_ui_->actionCaptureRefreshInterfaces, &QAction::triggered, this, [this]() {
        main_ui_->actionCaptureRefreshInterfaces->setEnabled(false);
        mainApp->refreshLocalInterfaces();
        main_ui_->actionCaptureRefreshInterfaces->setEnabled(true);
    });
#endif
}

void WiresharkMainWindow::showCaptureOptionsDialog()
{
#ifdef HAVE_LIBPCAP
    if (!capture_options_dialog_) {
        capture_options_dialog_ = new CaptureOptionsDialog(this);

        connect(capture_options_dialog_, &CaptureOptionsDialog::startCapture, this, [this]() { startCapture(); });
        connect(capture_options_dialog_, &CaptureOptionsDialog::stopCapture, this, &WiresharkMainWindow::stopCapture);

        connect(capture_options_dialog_, &CaptureOptionsDialog::interfacesChanged,
                this->welcome_page_, &WelcomePage::interfaceSelected);
        connect(capture_options_dialog_, &CaptureOptionsDialog::interfacesChanged,
                this->welcome_page_->getInterfaceFrame(), &InterfaceFrame::updateSelectedInterfaces);
        connect(capture_options_dialog_, &CaptureOptionsDialog::interfaceListChanged,
                this->welcome_page_->getInterfaceFrame(), &InterfaceFrame::interfaceListChanged);
        connect(capture_options_dialog_, &CaptureOptionsDialog::captureFilterTextEdited,
                this->welcome_page_, &WelcomePage::setCaptureFilterText);
        // Propagate selection changes from main UI to dialog.
        connect(this->welcome_page_, &WelcomePage::interfacesChanged,
                capture_options_dialog_, &CaptureOptionsDialog::interfaceSelected);

        connect(capture_options_dialog_, &CaptureOptionsDialog::setFilterValid,
                this, &WiresharkMainWindow::startInterfaceCapture);

        connect(capture_options_dialog_, &CaptureOptionsDialog::showExtcapOptions,
                this, &WiresharkMainWindow::showExtcapOptionsDialog);
    }
    capture_options_dialog_->updateInterfaces();

    if (capture_options_dialog_->isMinimized()) {
        capture_options_dialog_->showNormal();
    } else {
        capture_options_dialog_->show();
    }

    capture_options_dialog_->raise();
    capture_options_dialog_->activateWindow();
#endif
}

void WiresharkMainWindow::startCaptureTriggered()
{
//#ifdef HAVE_AIRPCAP
//  airpcap_if_active = airpcap_if_selected;
//  if (airpcap_if_active)
//    airpcap_set_toolbar_start_capture(airpcap_if_active);
//#endif

//  if (cap_open_w) {
//    /*
//     * There's an options dialog; get the values from it and close it.
//     */
//    gboolean success;

//    /* Determine if "capture start" while building of the "capture options" window */
//    /*  is in progress. If so, ignore the "capture start.                          */
//    /* XXX: Would it be better/cleaner for the "capture options" window code to    */
//    /*      disable the capture start button temporarily ?                         */
//    if (cap_open_complete == FALSE) {
//      return;  /* Building options window: ignore "capture start" */
//    }
//    success = capture_dlg_prep(cap_open_w);
//    window_destroy(GTK_WIDGET(cap_open_w));
//    if (!success)
//      return;   /* error in options dialog */
//  }

#ifdef HAVE_LIBPCAP
    if (global_capture_opts.num_selected == 0) {
        QString err_msg = tr("No Interface Selected.");
        mainApp->pushStatus(WiresharkApplication::TemporaryStatus, err_msg);
        main_ui_->actionCaptureStart->setChecked(false);
        return;
    }

    /* XXX - will closing this remove a temporary file? */
    QString before_what(tr(" before starting a new capture"));
    if (testCaptureFileClose(before_what)) {
        startCapture(QStringList());
    } else {
        // simply clicking the button sets it to 'checked' even though we've
        // decided to do nothing, so undo that
        main_ui_->actionCaptureStart->setChecked(false);
    }
#endif // HAVE_LIBPCAP
}

// Analyze Menu

void WiresharkMainWindow::filterMenuAboutToShow()
{
    QMenu * menu = qobject_cast<QMenu *>(sender());
    QString field_filter;

    if (capture_file_.capFile() && capture_file_.capFile()->finfo_selected) {
        char *tmp_field = proto_construct_match_selected_string(capture_file_.capFile()->finfo_selected,
                                                                capture_file_.capFile()->edt);
        field_filter = QString(tmp_field);
        wmem_free(NULL, tmp_field);
    }
    bool enable = ! field_filter.isEmpty();
    bool prepare = menu->objectName().compare("menuPrepareAFilter") == 0;

    menu->clear();
    QActionGroup * group = FilterAction::createFilterGroup(field_filter, prepare, enable, menu);
    menu->addActions(group->actions());
}

void WiresharkMainWindow::matchFieldFilter(FilterAction::Action action, FilterAction::ActionType filter_type)
{
    QString field_filter;

    if (packet_list_->contextMenuActive() || packet_list_->hasFocus()) {
        field_filter = packet_list_->getFilterFromRowAndColumn(packet_list_->currentIndex());
    } else if (capture_file_.capFile() && capture_file_.capFile()->finfo_selected) {
        char *tmp_field = proto_construct_match_selected_string(capture_file_.capFile()->finfo_selected,
                                                                capture_file_.capFile()->edt);
        field_filter = QString(tmp_field);
        wmem_free(NULL, tmp_field);
    }

    if (field_filter.isEmpty()) {
        QString err = tr("No filter available. Try another %1.").arg(packet_list_->contextMenuActive() ? tr("column") : tr("item"));
        mainApp->pushStatus(WiresharkApplication::TemporaryStatus, err);
        return;
    }

    setDisplayFilter(field_filter, action, filter_type);
}

void WiresharkMainWindow::on_actionAnalyzeDisplayFilters_triggered()
{
    if (!display_filter_dlg_) {
        display_filter_dlg_ = new FilterDialog(this, FilterDialog::DisplayFilter);
    }
    display_filter_dlg_->show();
    display_filter_dlg_->raise();
    display_filter_dlg_->activateWindow();
}

struct epan_uat;
void WiresharkMainWindow::on_actionAnalyzeDisplayFilterMacros_triggered()
{
    struct epan_uat* dfm_uat;
    dfilter_macro_get_uat(&dfm_uat);
    UatDialog *uat_dlg = new UatDialog(parentWidget(), dfm_uat);
    connect(uat_dlg, SIGNAL(destroyed(QObject*)), mainApp, SLOT(flushAppSignals()));

    uat_dlg->setWindowModality(Qt::ApplicationModal);
    uat_dlg->setAttribute(Qt::WA_DeleteOnClose);
    uat_dlg->show();
}

void WiresharkMainWindow::on_actionAnalyzeCreateAColumn_triggered()
{
    if (capture_file_.capFile() != 0 && capture_file_.capFile()->finfo_selected != 0) {
        header_field_info *hfinfo = capture_file_.capFile()->finfo_selected->hfinfo;
        int col = column_prefs_has_custom(hfinfo->abbrev);
        if (col == -1) {
            insertColumn(hfinfo->name, hfinfo->abbrev);
        } else {
            QString status;
            if (QString(hfinfo->name) == get_column_title(col)) {
                status = tr("The \"%1\" column already exists.").arg(hfinfo->name);
            } else {
                status = tr("The \"%1\" column already exists as \"%2\".").arg(hfinfo->name).arg(get_column_title(col));
            }
            mainApp->pushStatus(WiresharkApplication::TemporaryStatus, status);

            if (!get_column_visible(col)) {
                packet_list_->setColumnHidden(col, false);
                set_column_visible(col, TRUE);
                prefs_main_write();
            }
        }
    }
}

void WiresharkMainWindow::applyConversationFilter()
{
    ConversationAction *conv_action = qobject_cast<ConversationAction*>(sender());
    if (!conv_action) return;

    packet_info *pinfo = capture_file_.packetInfo();
    if (!pinfo) return;

    QByteArray conv_filter = conv_action->filter();
    if (conv_filter.isEmpty()) return;

    if (conv_action->isFilterValid(pinfo)) {

        df_combo_box_->lineEdit()->setText(conv_filter);
        df_combo_box_->applyDisplayFilter();
    }
}

void WiresharkMainWindow::applyExportObject()
{
    ExportObjectAction *export_action = qobject_cast<ExportObjectAction*>(sender());
    if (!export_action)
        return;

    ExportObjectDialog* export_dialog = new ExportObjectDialog(*this, capture_file_, export_action->exportObject());
    export_dialog->setWindowModality(Qt::ApplicationModal);
    export_dialog->setAttribute(Qt::WA_DeleteOnClose);
    export_dialog->show();
}

void WiresharkMainWindow::on_actionAnalyzeEnabledProtocols_triggered()
{
    EnabledProtocolsDialog *enable_proto_dialog = new EnabledProtocolsDialog(this);
    connect(enable_proto_dialog, SIGNAL(destroyed(QObject*)), mainApp, SLOT(flushAppSignals()));

    enable_proto_dialog->setWindowModality(Qt::ApplicationModal);
    enable_proto_dialog->setAttribute(Qt::WA_DeleteOnClose);
    enable_proto_dialog->show();
}

void WiresharkMainWindow::on_actionAnalyzeDecodeAs_triggered()
{
    QAction *da_action = qobject_cast<QAction*>(sender());
    bool create_new = da_action && da_action->property("create_new").toBool();

    DecodeAsDialog *da_dialog = new DecodeAsDialog(this, capture_file_.capFile(), create_new);
    connect(da_dialog, SIGNAL(destroyed(QObject*)), mainApp, SLOT(flushAppSignals()));

    da_dialog->setWindowModality(Qt::ApplicationModal);
    da_dialog->setAttribute(Qt::WA_DeleteOnClose);
    da_dialog->show();
}

void WiresharkMainWindow::on_actionAnalyzeReloadLuaPlugins_triggered()
{
    reloadLuaPlugins();
}

void WiresharkMainWindow::openFollowStreamDialog(follow_type_t type, guint stream_num, guint sub_stream_num, bool use_stream_index) {
    FollowStreamDialog *fsd = new FollowStreamDialog(*this, capture_file_, type);
    connect(fsd, SIGNAL(updateFilter(QString, bool)), this, SLOT(filterPackets(QString, bool)));
    connect(fsd, SIGNAL(goToPacket(int)), packet_list_, SLOT(goToPacket(int)));
    fsd->addCodecs(text_codec_map_);
    fsd->show();
    if (use_stream_index) {
        // If a specific conversation was requested, then ignore any previous
        // display filters and display all related packets.
        fsd->follow("", true, stream_num, sub_stream_num);
    } else {
        fsd->follow(getFilter());
    }
}

void WiresharkMainWindow::openFollowStreamDialogForType(follow_type_t type) {
    openFollowStreamDialog(type, 0, 0, false);
}

void WiresharkMainWindow::openSCTPAllAssocsDialog()
{
    SCTPAllAssocsDialog *sctp_dialog = new SCTPAllAssocsDialog(this, capture_file_.capFile());
    connect(sctp_dialog, SIGNAL(filterPackets(QString, bool)),
            this, SLOT(filterPackets(QString, bool)));
    connect(this, SIGNAL(setCaptureFile(capture_file*)),
            sctp_dialog, SLOT(setCaptureFile(capture_file*)));
    sctp_dialog->fillTable();

    if (sctp_dialog->isMinimized() == true)
    {
        sctp_dialog->showNormal();
    }
    else
    {
        sctp_dialog->show();
    }

    sctp_dialog->raise();
    sctp_dialog->activateWindow();
}

void WiresharkMainWindow::on_actionSCTPShowAllAssociations_triggered()
{
    openSCTPAllAssocsDialog();
}

void WiresharkMainWindow::on_actionSCTPAnalyseThisAssociation_triggered()
{
    const sctp_assoc_info_t* assoc = SCTPAssocAnalyseDialog::findAssocForPacket(capture_file_.capFile());
    if (!assoc) {
        return;
    }
    SCTPAssocAnalyseDialog *sctp_analyse = new SCTPAssocAnalyseDialog(this, assoc, capture_file_.capFile());
    connect(sctp_analyse, SIGNAL(filterPackets(QString, bool)),
            this, SLOT(filterPackets(QString, bool)));

    if (sctp_analyse->isMinimized() == true)
    {
        sctp_analyse->showNormal();
    }
    else
    {
        sctp_analyse->show();
    }

    sctp_analyse->raise();
    sctp_analyse->activateWindow();
}

void WiresharkMainWindow::on_actionSCTPFilterThisAssociation_triggered()
{
    const sctp_assoc_info_t* assoc = SCTPAssocAnalyseDialog::findAssocForPacket(capture_file_.capFile());
    if (assoc) {
        QString newFilter = QString("sctp.assoc_index==%1").arg(assoc->assoc_id);
        assoc = NULL;
        emit filterPackets(newFilter, false);
    }
}

// -z wlan,stat
void WiresharkMainWindow::statCommandWlanStatistics(const char *arg, void *)
{
    WlanStatisticsDialog *wlan_stats_dlg = new WlanStatisticsDialog(*this, capture_file_, arg);
    connect(wlan_stats_dlg, SIGNAL(filterAction(QString, FilterAction::Action, FilterAction::ActionType)),
            this, SIGNAL(filterAction(QString, FilterAction::Action, FilterAction::ActionType)));
    wlan_stats_dlg->show();
}

void WiresharkMainWindow::on_actionWirelessWlanStatistics_triggered()
{
    statCommandWlanStatistics(NULL, NULL);
}

// -z expert
void WiresharkMainWindow::statCommandExpertInfo(const char *, void *)
{
    const DisplayFilterEdit *df_edit = dynamic_cast<DisplayFilterEdit *>(df_combo_box_->lineEdit());
    ExpertInfoDialog *expert_dialog = new ExpertInfoDialog(*this, capture_file_, df_edit->text());

    connect(expert_dialog->getExpertInfoView(), SIGNAL(goToPacket(int, int)),
            packet_list_, SLOT(goToPacket(int, int)));
    connect(expert_dialog, SIGNAL(filterAction(QString, FilterAction::Action, FilterAction::ActionType)),
            this, SIGNAL(filterAction(QString, FilterAction::Action, FilterAction::ActionType)));

    expert_dialog->show();
}

void WiresharkMainWindow::on_actionAnalyzeShowPacketBytes_triggered()
{
    ShowPacketBytesDialog *spbd = new ShowPacketBytesDialog(*this, capture_file_);
    spbd->addCodecs(text_codec_map_);
    spbd->show();
}

void WiresharkMainWindow::on_actionAnalyzeExpertInfo_triggered()
{
    statCommandExpertInfo(NULL, NULL);
}


// Next / previous / first / last slots in packet_list

// Statistics Menu

void WiresharkMainWindow::on_actionStatisticsFlowGraph_triggered()
{
    SequenceDialog *sequence_dialog = new SequenceDialog(*this, capture_file_);
    sequence_dialog->show();
}

void WiresharkMainWindow::openTcpStreamDialog(int graph_type)
{
    TCPStreamDialog *stream_dialog = new TCPStreamDialog(this, capture_file_.capFile(), (tcp_graph_type)graph_type);
    connect(stream_dialog, SIGNAL(goToPacket(int)),
            packet_list_, SLOT(goToPacket(int)));
    connect(this, SIGNAL(setCaptureFile(capture_file*)),
            stream_dialog, SLOT(setCaptureFile(capture_file*)));
    if (stream_dialog->result() == QDialog::Accepted) {
        stream_dialog->show();
    }
}

void WiresharkMainWindow::on_actionStatisticsTcpStreamStevens_triggered()
{
    openTcpStreamDialog(GRAPH_TSEQ_STEVENS);
}

void WiresharkMainWindow::on_actionStatisticsTcpStreamTcptrace_triggered()
{
    openTcpStreamDialog(GRAPH_TSEQ_TCPTRACE);
}

void WiresharkMainWindow::on_actionStatisticsTcpStreamThroughput_triggered()
{
    openTcpStreamDialog(GRAPH_THROUGHPUT);
}

void WiresharkMainWindow::on_actionStatisticsTcpStreamRoundTripTime_triggered()
{
    openTcpStreamDialog(GRAPH_RTT);
}

void WiresharkMainWindow::on_actionStatisticsTcpStreamWindowScaling_triggered()
{
    openTcpStreamDialog(GRAPH_WSCALE);
}

// -z mcast,stat
void WiresharkMainWindow::statCommandMulticastStatistics(const char *arg, void *)
{
    MulticastStatisticsDialog *mcast_stats_dlg = new MulticastStatisticsDialog(*this, capture_file_, arg);
    connect(mcast_stats_dlg, SIGNAL(filterAction(QString, FilterAction::Action, FilterAction::ActionType)),
            this, SIGNAL(filterAction(QString, FilterAction::Action, FilterAction::ActionType)));
    mcast_stats_dlg->show();
}

void WiresharkMainWindow::on_actionStatisticsUdpMulticastStreams_triggered()
{
    statCommandMulticastStatistics(NULL, NULL);
}

void WiresharkMainWindow::openStatisticsTreeDialog(const gchar *abbr)
{
    StatsTreeDialog *st_dialog = new StatsTreeDialog(*this, capture_file_, abbr);
//    connect(st_dialog, SIGNAL(goToPacket(int)),
//            packet_list_, SLOT(goToPacket(int)));
    st_dialog->show();
}

void WiresharkMainWindow::on_actionStatistics29WestTopics_Advertisements_by_Topic_triggered()
{
    openStatisticsTreeDialog("lbmr_topic_ads_topic");
}

void WiresharkMainWindow::on_actionStatistics29WestTopics_Advertisements_by_Source_triggered()
{
    openStatisticsTreeDialog("lbmr_topic_ads_source");
}

void WiresharkMainWindow::on_actionStatistics29WestTopics_Advertisements_by_Transport_triggered()
{
    openStatisticsTreeDialog("lbmr_topic_ads_transport");
}

void WiresharkMainWindow::on_actionStatistics29WestTopics_Queries_by_Topic_triggered()
{
    openStatisticsTreeDialog("lbmr_topic_queries_topic");
}

void WiresharkMainWindow::on_actionStatistics29WestTopics_Queries_by_Receiver_triggered()
{
    openStatisticsTreeDialog("lbmr_topic_queries_receiver");
}

void WiresharkMainWindow::on_actionStatistics29WestTopics_Wildcard_Queries_by_Pattern_triggered()
{
    openStatisticsTreeDialog("lbmr_topic_queries_pattern");
}

void WiresharkMainWindow::on_actionStatistics29WestTopics_Wildcard_Queries_by_Receiver_triggered()
{
    openStatisticsTreeDialog("lbmr_topic_queries_pattern_receiver");
}

void WiresharkMainWindow::on_actionStatistics29WestQueues_Advertisements_by_Queue_triggered()
{
    openStatisticsTreeDialog("lbmr_queue_ads_queue");
}

void WiresharkMainWindow::on_actionStatistics29WestQueues_Advertisements_by_Source_triggered()
{
    openStatisticsTreeDialog("lbmr_queue_ads_source");
}

void WiresharkMainWindow::on_actionStatistics29WestQueues_Queries_by_Queue_triggered()
{
    openStatisticsTreeDialog("lbmr_queue_queries_queue");
}

void WiresharkMainWindow::on_actionStatistics29WestQueues_Queries_by_Receiver_triggered()
{
    openStatisticsTreeDialog("lbmr_queue_queries_receiver");
}

void WiresharkMainWindow::on_actionStatistics29WestUIM_Streams_triggered()
{
    LBMStreamDialog *stream_dialog = new LBMStreamDialog(this, capture_file_.capFile());
//    connect(stream_dialog, SIGNAL(goToPacket(int)),
//            packet_list_, SLOT(goToPacket(int)));
    connect(this, SIGNAL(setCaptureFile(capture_file*)),
            stream_dialog, SLOT(setCaptureFile(capture_file*)));
    stream_dialog->show();
}

void WiresharkMainWindow::on_actionStatistics29WestLBTRM_triggered()
{
    LBMLBTRMTransportDialog * lbtrm_dialog = new LBMLBTRMTransportDialog(this, capture_file_.capFile());
    connect(lbtrm_dialog, SIGNAL(goToPacket(int)),
            packet_list_, SLOT(goToPacket(int)));
    connect(this, SIGNAL(setCaptureFile(capture_file*)),
            lbtrm_dialog, SLOT(setCaptureFile(capture_file*)));
    lbtrm_dialog->show();
}
void WiresharkMainWindow::on_actionStatistics29WestLBTRU_triggered()
{
    LBMLBTRUTransportDialog * lbtru_dialog = new LBMLBTRUTransportDialog(this, capture_file_.capFile());
    connect(lbtru_dialog, SIGNAL(goToPacket(int)),
            packet_list_, SLOT(goToPacket(int)));
    connect(this, SIGNAL(setCaptureFile(capture_file*)),
            lbtru_dialog, SLOT(setCaptureFile(capture_file*)));
    lbtru_dialog->show();
}

void WiresharkMainWindow::on_actionStatisticsANCP_triggered()
{
    openStatisticsTreeDialog("ancp");
}


void WiresharkMainWindow::on_actionStatisticsBACappInstanceId_triggered()
{
    openStatisticsTreeDialog("bacapp_instanceid");
}

void WiresharkMainWindow::on_actionStatisticsBACappIP_triggered()
{
    openStatisticsTreeDialog("bacapp_ip");
}

void WiresharkMainWindow::on_actionStatisticsBACappObjectId_triggered()
{
    openStatisticsTreeDialog("bacapp_objectid");
}

void WiresharkMainWindow::on_actionStatisticsBACappService_triggered()
{
    openStatisticsTreeDialog("bacapp_service");
}

void WiresharkMainWindow::on_actionStatisticsCollectd_triggered()
{
    openStatisticsTreeDialog("collectd");
}

void WiresharkMainWindow::on_actionStatisticsConversations_triggered()
{
    ConversationDialog *conv_dialog = new ConversationDialog(*this, capture_file_);
    connect(conv_dialog, SIGNAL(filterAction(QString, FilterAction::Action, FilterAction::ActionType)),
        this, SIGNAL(filterAction(QString, FilterAction::Action, FilterAction::ActionType)));
    connect(conv_dialog, SIGNAL(openFollowStreamDialog(follow_type_t, guint, guint)),
        this, SLOT(openFollowStreamDialog(follow_type_t, guint, guint)));
    connect(conv_dialog, SIGNAL(openTcpStreamGraph(int)),
        this, SLOT(openTcpStreamDialog(int)));
    conv_dialog->show();
}

void WiresharkMainWindow::on_actionStatisticsEndpoints_triggered()
{
    EndpointDialog *endp_dialog = new EndpointDialog(*this, capture_file_);
    connect(endp_dialog, SIGNAL(filterAction(QString, FilterAction::Action, FilterAction::ActionType)),
            this, SIGNAL(filterAction(QString, FilterAction::Action, FilterAction::ActionType)));
    connect(endp_dialog, SIGNAL(openFollowStreamDialog(follow_type_t)),
            this, SLOT(openFollowStreamDialogForType(follow_type_t)));
    connect(endp_dialog, SIGNAL(openTcpStreamGraph(int)),
            this, SLOT(openTcpStreamDialog(int)));
    endp_dialog->show();
}

void WiresharkMainWindow::on_actionStatisticsHART_IP_triggered()
{
    openStatisticsTreeDialog("hart_ip");
}

void WiresharkMainWindow::on_actionStatisticsHpfeeds_triggered()
{
    openStatisticsTreeDialog("hpfeeds");
}

void WiresharkMainWindow::on_actionStatisticsHTTPPacketCounter_triggered()
{
    openStatisticsTreeDialog("http");
}

void WiresharkMainWindow::on_actionStatisticsHTTPRequests_triggered()
{
    openStatisticsTreeDialog("http_req");
}

void WiresharkMainWindow::on_actionStatisticsHTTPLoadDistribution_triggered()
{
    openStatisticsTreeDialog("http_srv");
}

void WiresharkMainWindow::on_actionStatisticsHTTPRequestSequences_triggered()
{
    openStatisticsTreeDialog("http_seq");
}

void WiresharkMainWindow::on_actionStatisticsPacketLengths_triggered()
{
    openStatisticsTreeDialog("plen");
}

void WiresharkMainWindow::on_actionStatisticsIOGraph_triggered()
{
    const DisplayFilterEdit *df_edit = qobject_cast<DisplayFilterEdit *>(df_combo_box_->lineEdit());
    QString displayFilter;
    if (df_edit)
        displayFilter = df_edit->text();

    IOGraphDialog *iog_dialog = new IOGraphDialog(*this, capture_file_, displayFilter);
    connect(iog_dialog, SIGNAL(goToPacket(int)), packet_list_, SLOT(goToPacket(int)));
    connect(this, SIGNAL(reloadFields()), iog_dialog, SLOT(reloadFields()));
    iog_dialog->show();
}

void WiresharkMainWindow::on_actionStatisticsSametime_triggered()
{
    openStatisticsTreeDialog("sametime");
}

void WiresharkMainWindow::on_actionStatisticsDNS_triggered()
{
    openStatisticsTreeDialog("dns");
}

void WiresharkMainWindow::on_actionStatisticsHTTP2_triggered()
{
    openStatisticsTreeDialog("http2");

}

void WiresharkMainWindow::on_actionStatisticsSOMEIPmessages_triggered()
{
    openStatisticsTreeDialog("someip_messages");
}

void WiresharkMainWindow::on_actionStatisticsSOMEIPSDentries_triggered()
{
    openStatisticsTreeDialog("someipsd_entries");
}

// Telephony Menu

RtpPlayerDialog *WiresharkMainWindow::openTelephonyRtpPlayerDialog()
{
    RtpPlayerDialog *dialog;

#ifdef HAVE_LIBPCAP
    dialog = RtpPlayerDialog::openRtpPlayerDialog(*this, capture_file_, packet_list_, captureSession()->state != CAPTURE_STOPPED);
#else
    dialog = RtpPlayerDialog::openRtpPlayerDialog(*this, capture_file_, packet_list_, false);
#endif

    dialog->show();

    return dialog;
}

VoipCallsDialog *WiresharkMainWindow::openTelephonyVoipCallsDialogVoip()
{
    VoipCallsDialog *dialog;

    dialog = VoipCallsDialog::openVoipCallsDialogVoip(*this, capture_file_, packet_list_);
    dialog->show();

    return dialog;
}

VoipCallsDialog *WiresharkMainWindow::openTelephonyVoipCallsDialogSip()
{
    VoipCallsDialog *dialog;

    dialog = VoipCallsDialog::openVoipCallsDialogSip(*this, capture_file_, packet_list_);
    dialog->show();

    return dialog;
}

RtpAnalysisDialog *WiresharkMainWindow::openTelephonyRtpAnalysisDialog()
{
    RtpAnalysisDialog *dialog;

    dialog = RtpAnalysisDialog::openRtpAnalysisDialog(*this, capture_file_, packet_list_);
    dialog->show();

    return dialog;
}

void WiresharkMainWindow::on_actionTelephonyVoipCalls_triggered()
{
    openTelephonyVoipCallsDialogVoip();
}

void WiresharkMainWindow::on_actionTelephonyGsmMapSummary_triggered()
{
    GsmMapSummaryDialog *gms_dialog = new GsmMapSummaryDialog(*this, capture_file_);
    gms_dialog->show();
}

void WiresharkMainWindow::on_actionTelephonyIax2StreamAnalysis_triggered()
{
    Iax2AnalysisDialog *iax2_analysis_dialog = new  Iax2AnalysisDialog(*this, capture_file_);
    connect(iax2_analysis_dialog, SIGNAL(goToPacket(int)),
            packet_list_, SLOT(goToPacket(int)));
    iax2_analysis_dialog->show();
}

void WiresharkMainWindow::on_actionTelephonyISUPMessages_triggered()
{
    openStatisticsTreeDialog("isup_msg");
}

// -z mac-lte,stat
void WiresharkMainWindow::statCommandLteMacStatistics(const char *arg, void *)
{
    LteMacStatisticsDialog *lte_mac_stats_dlg = new LteMacStatisticsDialog(*this, capture_file_, arg);
    connect(lte_mac_stats_dlg, SIGNAL(filterAction(QString, FilterAction::Action, FilterAction::ActionType)),
            this, SIGNAL(filterAction(QString, FilterAction::Action, FilterAction::ActionType)));
    lte_mac_stats_dlg->show();
}

void WiresharkMainWindow::on_actionTelephonyLteMacStatistics_triggered()
{
    statCommandLteMacStatistics(NULL, NULL);
}

void WiresharkMainWindow::statCommandLteRlcStatistics(const char *arg, void *)
{
    LteRlcStatisticsDialog *lte_rlc_stats_dlg = new LteRlcStatisticsDialog(*this, capture_file_, arg);
    connect(lte_rlc_stats_dlg, SIGNAL(filterAction(QString, FilterAction::Action, FilterAction::ActionType)),
            this, SIGNAL(filterAction(QString, FilterAction::Action, FilterAction::ActionType)));
    // N.B. It is necessary for the RLC Statistics window to launch the RLC graph in this way, to ensure
    // that the goToPacket() signal/slot connection gets set up...
    connect(lte_rlc_stats_dlg, SIGNAL(launchRLCGraph(bool, guint16, guint8, guint16, guint16, guint8)),
            this, SLOT(launchRLCGraph(bool, guint16, guint8, guint16, guint16, guint8)));

    lte_rlc_stats_dlg->show();
}

void WiresharkMainWindow::on_actionTelephonyLteRlcStatistics_triggered()
{
    statCommandLteRlcStatistics(NULL, NULL);
}

void WiresharkMainWindow::launchRLCGraph(bool channelKnown,
    guint16 ueid, guint8 rlcMode,
    guint16 channelType, guint16 channelId, guint8 direction)
{
    LteRlcGraphDialog *lrg_dialog = new LteRlcGraphDialog(*this, capture_file_, channelKnown);
    connect(lrg_dialog, SIGNAL(goToPacket(int)), packet_list_, SLOT(goToPacket(int)));
    // This is a bit messy, but wanted to hide these parameters from users of
    // on_actionTelephonyLteRlcGraph_triggered().
    if (channelKnown) {
        lrg_dialog->setChannelInfo(ueid, rlcMode, channelType, channelId, direction);
    }
    lrg_dialog->show();
}

void WiresharkMainWindow::on_actionTelephonyLteRlcGraph_triggered()
{
    // We don't yet know the channel.
    launchRLCGraph(false, 0, 0, 0, 0, 0);
}

void WiresharkMainWindow::on_actionTelephonyMtp3Summary_triggered()
{
    Mtp3SummaryDialog *mtp3s_dialog = new Mtp3SummaryDialog(*this, capture_file_);
    mtp3s_dialog->show();
}

void WiresharkMainWindow::on_actionTelephonyOsmuxPacketCounter_triggered()
{
    openStatisticsTreeDialog("osmux");
}

RtpStreamDialog *WiresharkMainWindow::openTelephonyRtpStreamsDialog()
{
    RtpStreamDialog *dialog;

    dialog = RtpStreamDialog::openRtpStreamDialog(*this, capture_file_, packet_list_);
    dialog->show();

    return dialog;
}

void WiresharkMainWindow::on_actionTelephonyRtpStreams_triggered()
{
    openTelephonyRtpStreamsDialog();
}

void WiresharkMainWindow::on_actionTelephonyRtpStreamAnalysis_triggered()
{
    QVector<rtpstream_id_t *> stream_ids;
    QString err;

    if (QGuiApplication::keyboardModifiers().testFlag(Qt::ControlModifier)) {
        err = findRtpStreams(&stream_ids, true);
    } else {
        err = findRtpStreams(&stream_ids, false);
    }
    if (!err.isNull()) {
        QMessageBox::warning(this, tr("RTP packet search failed"),
                             err,
                             QMessageBox::Ok);
    } else {
        openTelephonyRtpAnalysisDialog()->addRtpStreams(stream_ids);
    }
    foreach(rtpstream_id_t *id, stream_ids) {
        rtpstream_id_free(id);
    }
}

void WiresharkMainWindow::on_actionTelephonyRtpPlayer_triggered()
{
    QVector<rtpstream_id_t *> stream_ids;
    QString err;

    if (QGuiApplication::keyboardModifiers().testFlag(Qt::ControlModifier)) {
        err = findRtpStreams(&stream_ids, true);
    } else {
        err = findRtpStreams(&stream_ids, false);
    }
    if (!err.isNull()) {
        QMessageBox::warning(this, tr("RTP packet search failed"),
                             err,
                             QMessageBox::Ok);
#ifdef QT_MULTIMEDIA_LIB
    } else {
        openTelephonyRtpPlayerDialog()->addRtpStreams(stream_ids);
#endif // QT_MULTIMEDIA_LIB
    }
    foreach(rtpstream_id_t *id, stream_ids) {
        rtpstream_id_free(id);
    }
}

void WiresharkMainWindow::on_actionTelephonyRTSPPacketCounter_triggered()
{
    openStatisticsTreeDialog("rtsp");
}

void WiresharkMainWindow::on_actionTelephonySMPPOperations_triggered()
{
    openStatisticsTreeDialog("smpp_commands");
}

void WiresharkMainWindow::on_actionTelephonyUCPMessages_triggered()
{
    openStatisticsTreeDialog("ucp_messages");
}

void WiresharkMainWindow::on_actionTelephonyF1APMessages_triggered()
{
	openStatisticsTreeDialog("f1ap");
}

void WiresharkMainWindow::on_actionTelephonyNGAPMessages_triggered()
{
    openStatisticsTreeDialog("ngap");
}

void WiresharkMainWindow::on_actionTelephonySipFlows_triggered()
{
    openTelephonyVoipCallsDialogSip();
}

// Wireless Menu

void WiresharkMainWindow::on_actionBluetoothATT_Server_Attributes_triggered()
{
    BluetoothAttServerAttributesDialog *bluetooth_att_sever_attributes_dialog = new BluetoothAttServerAttributesDialog(*this, capture_file_);
    connect(bluetooth_att_sever_attributes_dialog, SIGNAL(goToPacket(int)),
            packet_list_, SLOT(goToPacket(int)));
    connect(bluetooth_att_sever_attributes_dialog, SIGNAL(updateFilter(QString, bool)),
            this, SLOT(filterPackets(QString, bool)));
    bluetooth_att_sever_attributes_dialog->show();
}

void WiresharkMainWindow::on_actionBluetoothDevices_triggered()
{
    BluetoothDevicesDialog *bluetooth_devices_dialog = new BluetoothDevicesDialog(*this, capture_file_, packet_list_);
    connect(bluetooth_devices_dialog, SIGNAL(goToPacket(int)),
            packet_list_, SLOT(goToPacket(int)));
    connect(bluetooth_devices_dialog, SIGNAL(updateFilter(QString, bool)),
            this, SLOT(filterPackets(QString, bool)));
    bluetooth_devices_dialog->show();
}

void WiresharkMainWindow::on_actionBluetoothHCI_Summary_triggered()
{
    BluetoothHciSummaryDialog *bluetooth_hci_summary_dialog = new BluetoothHciSummaryDialog(*this, capture_file_);
    connect(bluetooth_hci_summary_dialog, SIGNAL(goToPacket(int)),
            packet_list_, SLOT(goToPacket(int)));
    connect(bluetooth_hci_summary_dialog, SIGNAL(updateFilter(QString, bool)),
            this, SLOT(filterPackets(QString, bool)));
    bluetooth_hci_summary_dialog->show();
}

// Tools Menu

void WiresharkMainWindow::on_actionToolsFirewallAclRules_triggered()
{
    FirewallRulesDialog *firewall_rules_dialog = new FirewallRulesDialog(*this, capture_file_);
    firewall_rules_dialog->show();
}

void WiresharkMainWindow::on_actionToolsCredentials_triggered()
{
    CredentialsDialog *credentials_dialog = new CredentialsDialog(*this, capture_file_, packet_list_);
    credentials_dialog->show();
}

// Help Menu
void WiresharkMainWindow::on_actionHelpContents_triggered() {

    mainApp->helpTopicAction(HELP_CONTENT);
}

void WiresharkMainWindow::on_actionHelpMPWireshark_triggered() {

    mainApp->helpTopicAction(LOCALPAGE_MAN_WIRESHARK);
}

void WiresharkMainWindow::on_actionHelpMPWireshark_Filter_triggered() {
    mainApp->helpTopicAction(LOCALPAGE_MAN_WIRESHARK_FILTER);
}

void WiresharkMainWindow::on_actionHelpMPCapinfos_triggered() {
    mainApp->helpTopicAction(LOCALPAGE_MAN_CAPINFOS);
}

void WiresharkMainWindow::on_actionHelpMPDumpcap_triggered() {
    mainApp->helpTopicAction(LOCALPAGE_MAN_DUMPCAP);
}

void WiresharkMainWindow::on_actionHelpMPEditcap_triggered() {
    mainApp->helpTopicAction(LOCALPAGE_MAN_EDITCAP);
}

void WiresharkMainWindow::on_actionHelpMPMergecap_triggered() {
    mainApp->helpTopicAction(LOCALPAGE_MAN_MERGECAP);
}

void WiresharkMainWindow::on_actionHelpMPRawshark_triggered() {
    mainApp->helpTopicAction(LOCALPAGE_MAN_RAWSHARK);
}

void WiresharkMainWindow::on_actionHelpMPReordercap_triggered() {
    mainApp->helpTopicAction(LOCALPAGE_MAN_REORDERCAP);
}

void WiresharkMainWindow::on_actionHelpMPText2pcap_triggered() {
    mainApp->helpTopicAction(LOCALPAGE_MAN_TEXT2PCAP);
}

void WiresharkMainWindow::on_actionHelpMPTShark_triggered() {
    mainApp->helpTopicAction(LOCALPAGE_MAN_TSHARK);
}

void WiresharkMainWindow::on_actionHelpWebsite_triggered() {

    mainApp->helpTopicAction(ONLINEPAGE_HOME);
}

void WiresharkMainWindow::on_actionHelpFAQ_triggered() {

    mainApp->helpTopicAction(ONLINEPAGE_FAQ);
}

void WiresharkMainWindow::on_actionHelpAsk_triggered() {

    mainApp->helpTopicAction(ONLINEPAGE_ASK);
}

void WiresharkMainWindow::on_actionHelpDownloads_triggered() {

    mainApp->helpTopicAction(ONLINEPAGE_DOWNLOAD);
}

void WiresharkMainWindow::on_actionHelpWiki_triggered() {

    mainApp->helpTopicAction(ONLINEPAGE_WIKI);
}

void WiresharkMainWindow::on_actionHelpSampleCaptures_triggered() {

    mainApp->helpTopicAction(ONLINEPAGE_SAMPLE_FILES);
}

#ifdef HAVE_SOFTWARE_UPDATE
void WiresharkMainWindow::checkForUpdates()
{
    software_update_check();
}
#endif

void WiresharkMainWindow::on_actionHelpAbout_triggered()
{
    AboutDialog *about_dialog = new AboutDialog(this);

    if (about_dialog->isMinimized() == true)
    {
        about_dialog->showNormal();
    }
    else
    {
        about_dialog->show();
    }

    about_dialog->raise();
    about_dialog->activateWindow();
}

void WiresharkMainWindow::resetPreviousFocus() {
    previous_focus_ = NULL;
}

void WiresharkMainWindow::on_goToCancel_clicked()
{
    main_ui_->goToFrame->animatedHide();
    if (previous_focus_) {
        disconnect(previous_focus_, SIGNAL(destroyed()), this, SLOT(resetPreviousFocus()));
        previous_focus_->setFocus();
        resetPreviousFocus();
    }
}

void WiresharkMainWindow::on_goToGo_clicked()
{
    gotoFrame(main_ui_->goToLineEdit->text().toInt());

    on_goToCancel_clicked();
}

void WiresharkMainWindow::on_goToLineEdit_returnPressed()
{
    on_goToGo_clicked();
}

void WiresharkMainWindow::on_actionStatisticsCaptureFileProperties_triggered()
{
    CaptureFilePropertiesDialog *capture_file_properties_dialog = new CaptureFilePropertiesDialog(*this, capture_file_);
    connect(capture_file_properties_dialog, SIGNAL(captureCommentChanged()),
            this, SLOT(updateForUnsavedChanges()));
    capture_file_properties_dialog->show();
}

void WiresharkMainWindow::on_actionStatisticsResolvedAddresses_triggered()
{
    QString capFileName;
    wtap* wth = Q_NULLPTR;
    if (capture_file_.isValid())
    {
        capFileName = capture_file_.capFile()->filename;
        wth = capture_file_.capFile()->provider.wth;
    }
    ResolvedAddressesDialog *resolved_addresses_dialog =
            new ResolvedAddressesDialog(this, capFileName, wth);
    resolved_addresses_dialog->show();
}

void WiresharkMainWindow::on_actionStatisticsProtocolHierarchy_triggered()
{
    ProtocolHierarchyDialog *phd = new ProtocolHierarchyDialog(*this, capture_file_);
    connect(phd, SIGNAL(filterAction(QString, FilterAction::Action, FilterAction::ActionType)),
            this, SIGNAL(filterAction(QString, FilterAction::Action, FilterAction::ActionType)));
    phd->show();
}

void WiresharkMainWindow::externalMenuItem_triggered()
{
    QAction * triggerAction = NULL;
    QVariant v;
    ext_menubar_t * entry = NULL;

    if (QObject::sender()) {
        triggerAction = (QAction *)QObject::sender();
        v = triggerAction->data();

        if (v.canConvert<void *>()) {
            entry = (ext_menubar_t *)v.value<void *>();

            if (entry->type == EXT_MENUBAR_ITEM) {
                entry->callback(EXT_MENUBAR_QT_GUI, (gpointer)((void *)main_ui_), entry->user_data);
            } else {
                QDesktopServices::openUrl(QUrl(QString((gchar *)entry->user_data)));
            }
        }
    }
}

void WiresharkMainWindow::extcap_options_finished(int result)
{
    if (result == QDialog::Accepted) {
        QString before_what(tr(" before starting a new capture"));
        if (testCaptureFileClose(before_what)) {
            startCapture(QStringList());
        }
    }
    this->welcome_page_->getInterfaceFrame()->interfaceListChanged();
}

void WiresharkMainWindow::showExtcapOptionsDialog(QString &device_name, bool startCaptureOnClose)
{
    ExtcapOptionsDialog * extcap_options_dialog = ExtcapOptionsDialog::createForDevice(device_name, startCaptureOnClose, this);
    /* The dialog returns null, if the given device name is not a valid extcap device */
    if (extcap_options_dialog) {
        extcap_options_dialog->setModal(true);
        extcap_options_dialog->setAttribute(Qt::WA_DeleteOnClose);
        if (startCaptureOnClose) {
            connect(extcap_options_dialog, SIGNAL(finished(int)),
                        this, SLOT(extcap_options_finished(int)));
        }
#ifdef HAVE_LIBPCAP
        if (capture_options_dialog_ && startCaptureOnClose) {
            /* Allow capture options dialog to close */
            connect(extcap_options_dialog, SIGNAL(accepted()),
                    capture_options_dialog_, SLOT(accept()));
        }
#endif
        extcap_options_dialog->show();
    }
}

void WiresharkMainWindow::on_actionContextWikiProtocolPage_triggered()
{
    QAction *wa = qobject_cast<QAction*>(sender());
    if (!wa) return;

    bool ok = false;
    int field_id = wa->data().toInt(&ok);
    if (!ok) return;

    const QString proto_abbrev = proto_registrar_get_abbrev(field_id);

    int ret = QMessageBox::question(this, mainApp->windowTitleString(tr("Wiki Page for %1").arg(proto_abbrev)),
                                    tr("<p>The Wireshark Wiki is maintained by the community.</p>"
                                    "<p>The page you are about to load might be wonderful, "
                                    "incomplete, wrong, or nonexistent.</p>"
                                    "<p>Proceed to the wiki?</p>"),
                                    QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);

    if (ret != QMessageBox::Yes) return;

    QUrl wiki_url = QString(WS_WIKI_URL("Protocols/%1")).arg(proto_abbrev);
    QDesktopServices::openUrl(wiki_url);
}

void WiresharkMainWindow::on_actionContextFilterFieldReference_triggered()
{
    QAction *wa = qobject_cast<QAction*>(sender());
    if (!wa) return;

    bool ok = false;
    int field_id = wa->data().toInt(&ok);
    if (!ok) return;

    const QString proto_abbrev = proto_registrar_get_abbrev(field_id);

    QUrl dfref_url = QString(WS_DOCS_URL "/dfref/%1/%2")
            .arg(proto_abbrev[0])
            .arg(proto_abbrev);
    QDesktopServices::openUrl(dfref_url);
}

void WiresharkMainWindow::activatePluginIFToolbar(bool)
{
    QAction *sendingAction = dynamic_cast<QAction *>(sender());
    if (!sendingAction || !sendingAction->data().isValid())
        return;

    ext_toolbar_t *toolbar = VariantPointer<ext_toolbar_t>::asPtr(sendingAction->data());

    QList<QToolBar *> toolbars = findChildren<QToolBar *>();
    foreach(QToolBar *bar, toolbars) {
        AdditionalToolBar *iftoolbar = dynamic_cast<AdditionalToolBar *>(bar);
        if (iftoolbar && iftoolbar->menuName().compare(toolbar->name) == 0) {
            if (iftoolbar->isVisible()) {
                iftoolbar->setVisible(false);
                sendingAction->setChecked(true);
            } else {
                iftoolbar->setVisible(true);
                sendingAction->setChecked(true);
            }
        }
    }
}

void WiresharkMainWindow::rtpPlayerDialogReplaceRtpStreams(QVector<rtpstream_id_t *> stream_ids _U_)
{
#ifdef QT_MULTIMEDIA_LIB
    openTelephonyRtpPlayerDialog()->replaceRtpStreams(stream_ids);
#endif
}

void WiresharkMainWindow::rtpPlayerDialogAddRtpStreams(QVector<rtpstream_id_t *> stream_ids _U_)
{
#ifdef QT_MULTIMEDIA_LIB
    openTelephonyRtpPlayerDialog()->addRtpStreams(stream_ids);
#endif
}

void WiresharkMainWindow::rtpPlayerDialogRemoveRtpStreams(QVector<rtpstream_id_t *> stream_ids _U_)
{
#ifdef QT_MULTIMEDIA_LIB
    openTelephonyRtpPlayerDialog()->removeRtpStreams(stream_ids);
#endif
}

void WiresharkMainWindow::rtpAnalysisDialogReplaceRtpStreams(QVector<rtpstream_id_t *> stream_ids)
{
    openTelephonyRtpAnalysisDialog()->replaceRtpStreams(stream_ids);
}

void WiresharkMainWindow::rtpAnalysisDialogAddRtpStreams(QVector<rtpstream_id_t *> stream_ids)
{
    openTelephonyRtpAnalysisDialog()->addRtpStreams(stream_ids);
}

void WiresharkMainWindow::rtpAnalysisDialogRemoveRtpStreams(QVector<rtpstream_id_t *> stream_ids)
{
    openTelephonyRtpAnalysisDialog()->removeRtpStreams(stream_ids);
}

void WiresharkMainWindow::rtpStreamsDialogSelectRtpStreams(QVector<rtpstream_id_t *> stream_ids)
{
    openTelephonyRtpStreamsDialog()->selectRtpStream(stream_ids);
}

void WiresharkMainWindow::rtpStreamsDialogDeselectRtpStreams(QVector<rtpstream_id_t *> stream_ids)
{
    openTelephonyRtpStreamsDialog()->deselectRtpStream(stream_ids);
}

#ifdef _MSC_VER
#pragma warning(pop)
#endif
