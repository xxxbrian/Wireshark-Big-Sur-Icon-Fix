/* timeline_delegate.cpp
 *
 * Wireshark - Network traffic analyzer
 * By Gerald Combs <gerald@wireshark.org>
 * Copyright 1998 Gerald Combs
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <ui/qt/models/timeline_delegate.h>
#include <ui/qt/models/atap_data_model.h>

#include <ui/qt/utils/color_utils.h>

#include <QApplication>
#include <QPainter>
#include <QTreeView>

// XXX We might want to move this to conversation_dialog.cpp.

// PercentBarDelegate uses a stronger blend value, but its bars are also
// more of a prominent feature. Make the blend weaker here so that we don't
// obscure our text.
static const double bar_blend_ = 0.08;

TimelineDelegate::TimelineDelegate(QWidget *parent) :
    QStyledItemDelegate(parent)
{
    _dataRole = Qt::UserRole;
}

void TimelineDelegate::setDataRole(int dataRole)
{
    _dataRole = dataRole;
}

void TimelineDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                               const QModelIndex &index) const
{
    QStyleOptionViewItem option_vi = option;
    QStyledItemDelegate::initStyleOption(&option_vi, index);

    struct timeline_span span_px = index.data(_dataRole).value<struct timeline_span>();
    if (_dataRole == ATapDataModel::TIMELINE_DATA) {
        double span_s = span_px.maxRelTime - span_px.minRelTime;
        if (qobject_cast<QTreeView *>(parent()) == nullptr)
            return;
        QTreeView * tree = qobject_cast<QTreeView *>(parent());
        int start_px = tree->columnWidth(span_px.colStart);
        int column_px = start_px + tree->columnWidth(span_px.colDuration);

        span_px.start = ((span_px.startTime - span_px.minRelTime) * column_px) / span_s;
        span_px.width = ((span_px.stopTime - span_px.startTime) * column_px) / span_s;

        if (index.column() == span_px.colDuration) {
            span_px.start -= start_px;
        }
    }

    // Paint our rect with no text using the current style, then draw our
    // bar and text over it.
    QStyledItemDelegate::paint(painter, option, index);

    if (QApplication::style()->objectName().contains("vista")) {
        // QWindowsVistaStyle::drawControl does this internally. Unfortunately there
        // doesn't appear to be a more general way to do this.
        option_vi.palette.setColor(QPalette::All, QPalette::HighlightedText,
                               option_vi.palette.color(QPalette::Active, QPalette::Text));
    }

    QPalette::ColorGroup cg = option_vi.state & QStyle::State_Enabled
                              ? QPalette::Normal : QPalette::Disabled;
    QColor text_color = option_vi.palette.color(cg, QPalette::Text);
    QColor bar_color = ColorUtils::alphaBlend(option_vi.palette.windowText(),
                                              option_vi.palette.window(), bar_blend_);

    if (cg == QPalette::Normal && !(option_vi.state & QStyle::State_Active))
        cg = QPalette::Inactive;
    if (option_vi.state & QStyle::State_Selected) {
        text_color = option_vi.palette.color(cg, QPalette::HighlightedText);
        bar_color = ColorUtils::alphaBlend(option_vi.palette.color(cg, QPalette::Window),
                                           option_vi.palette.color(cg, QPalette::Highlight),
                                           bar_blend_);
    }

    painter->save();
    int border_radius = 3; // We use 3 px elsewhere, e.g. filter combos.
    QRect timeline_rect = option.rect;
    timeline_rect.adjust(span_px.start, 1, 0, -1);
    timeline_rect.setWidth(span_px.width);
    painter->setClipRect(option.rect);
    painter->setPen(Qt::NoPen);
    painter->setBrush(bar_color);
    painter->drawRoundedRect(timeline_rect, border_radius, border_radius);
    painter->restore();

    painter->save();
    painter->setPen(text_color);
    painter->drawText(option.rect, Qt::AlignCenter, index.data(Qt::DisplayRole).toString());
    painter->restore();
}
