/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2015 - 2017 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program. If not, see <http://www.gnu.org/licenses/>.
*
**************************************************************************/

#include "TabSwitcherWidget.h"
#include "Window.h"
#include "../core/ActionsManager.h"
#include "../core/ThemesManager.h"
#include "../core/WindowsManager.h"

#include <QtGui/QKeyEvent>
#include <QtGui/QMovie>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>

namespace Otter
{

TabSwitcherWidget::TabSwitcherWidget(WindowsManager *manager, QWidget *parent) : QWidget(parent),
	m_windowsManager(manager),
	m_model(new QStandardItemModel(this)),
	m_frame(new QFrame(this)),
	m_tabsView(new ItemViewWidget(m_frame)),
	m_previewLabel(new QLabel(m_frame)),
	m_loadingMovie(nullptr),
	m_reason(KeyboardReason)
{
	QHBoxLayout *mainLayout(new QHBoxLayout(this));
	mainLayout->addWidget(m_frame, 0, Qt::AlignCenter);

	setLayout(mainLayout);
	setAutoFillBackground(false);

	QHBoxLayout *frameLayout(new QHBoxLayout(m_frame));
	frameLayout->addWidget(m_tabsView, 1);
	frameLayout->addWidget(m_previewLabel, 0, Qt::AlignCenter);

	m_model->setSortRole(Qt::UserRole);

	m_frame->setLayout(frameLayout);
	m_frame->setAutoFillBackground(true);
	m_frame->setMinimumWidth(600);
	m_frame->setObjectName(QLatin1String("tabSwitcher"));
	m_frame->setStyleSheet(QStringLiteral("#tabSwitcher {background:%1;border:1px solid #B3B3B3;border-radius:4px;}").arg(palette().color(QPalette::Base).name()));

	m_tabsView->setModel(m_model);
	m_tabsView->setStyleSheet(QLatin1String("border:0;"));
	m_tabsView->header()->hide();
	m_tabsView->header()->setStretchLastSection(true);
	m_tabsView->viewport()->installEventFilter(this);

	m_previewLabel->setFixedSize(260, 170);
	m_previewLabel->setAlignment(Qt::AlignCenter);
	m_previewLabel->setStyleSheet(QLatin1String("border:1px solid gray;"));

	connect(m_tabsView->selectionModel(), SIGNAL(currentChanged(QModelIndex,QModelIndex)), this, SLOT(handleCurrentTabChanged(QModelIndex)));
}

void TabSwitcherWidget::showEvent(QShowEvent *event)
{
	grabKeyboard();

	for (int i = 0; i < m_windowsManager->getWindowCount(); ++i)
	{
		Window *window(m_windowsManager->getWindowByIndex(i));

		if (window)
		{
			m_model->appendRow(createRow(window));
		}
	}

	m_model->sort(0, Qt::DescendingOrder);

	m_tabsView->setCurrentIndex(m_model->index(0, 0));

	const int contentsHeight(m_model->rowCount() * 22);

	m_tabsView->setMinimumHeight(qMin(contentsHeight, int(height() * 0.9)));

	QWidget::showEvent(event);

	connect(m_windowsManager, SIGNAL(windowAdded(quint64)), this, SLOT(handleWindowAdded(quint64)));
	connect(m_windowsManager, SIGNAL(windowRemoved(quint64)), this, SLOT(handleWindowRemoved(quint64)));
}

void TabSwitcherWidget::hideEvent(QHideEvent *event)
{
	releaseKeyboard();

	QWidget::hideEvent(event);

	disconnect(m_windowsManager, SIGNAL(windowAdded(quint64)), this, SLOT(handleWindowAdded(quint64)));
	disconnect(m_windowsManager, SIGNAL(windowRemoved(quint64)), this, SLOT(handleWindowRemoved(quint64)));

	m_model->clear();
}

void TabSwitcherWidget::keyPressEvent(QKeyEvent *event)
{
	if (event->key() == Qt::Key_Tab || event->key() == Qt::Key_Down)
	{
		selectTab(true);
	}
	else if (event->key() == Qt::Key_Backtab || event->key() == Qt::Key_Up)
	{
		selectTab(false);
	}
	else if (event->key() == Qt::Key_Enter || event->key() == Qt::Key_Return)
	{
		accept();
	}
	else if (event->key() == Qt::Key_Escape)
	{
		hide();
	}
}

void TabSwitcherWidget::keyReleaseEvent(QKeyEvent *event)
{
	if (event->key() == Qt::Key_Control && m_reason == KeyboardReason)
	{
		accept();

		event->accept();
	}
}

void TabSwitcherWidget::handleCurrentTabChanged(const QModelIndex &index)
{
	Window *window(m_windowsManager->getWindowByIdentifier(index.data(Qt::UserRole).toULongLong()));

	m_previewLabel->setMovie(nullptr);
	m_previewLabel->setPixmap(QPixmap());

	if (!window)
	{
		return;
	}

	if (window->getLoadingState() == WebWidget::DelayedLoadingState || window->getLoadingState() == WebWidget::OngoingLoadingState)
	{
		if (!m_loadingMovie)
		{
			m_loadingMovie = new QMovie(QLatin1String(":/icons/loading.gif"), QByteArray(), m_previewLabel);
			m_loadingMovie->start();
		}

		m_previewLabel->setMovie(m_loadingMovie);

		m_loadingMovie->setSpeed((window->getLoadingState() == WebWidget::OngoingLoadingState) ? 100 : 10);
	}
	else
	{
		m_previewLabel->setPixmap((window->getLoadingState() == WebWidget::CrashedLoadingState) ? ThemesManager::getIcon(QLatin1String("tab-crashed")).pixmap(32, 32) : window->getThumbnail());
	}
}

void TabSwitcherWidget::handleWindowAdded(quint64 identifier)
{
	Window *window(m_windowsManager->getWindowByIdentifier(identifier));

	if (window)
	{
		m_model->insertRow(0, createRow(window));
	}
}

void TabSwitcherWidget::handleWindowRemoved(quint64 identifier)
{
	const int row(findRow(identifier));

	if (row >= 0)
	{
		m_model->removeRow(row);
	}
}

void TabSwitcherWidget::show(SwitcherReason reason)
{
	m_reason = reason;

	QWidget::show();
}

void TabSwitcherWidget::accept()
{
	m_windowsManager->setActiveWindowByIdentifier(m_tabsView->currentIndex().data(Qt::UserRole).toULongLong());

	hide();
}

void TabSwitcherWidget::selectTab(bool next)
{
	const int currentRow(m_tabsView->currentIndex().row());

	m_tabsView->setCurrentIndex(m_model->index((next ? ((currentRow == (m_model->rowCount() - 1)) ? 0 : (currentRow + 1)) : ((currentRow == 0) ? (m_model->rowCount() - 1) : (currentRow - 1))), 0));
}

void TabSwitcherWidget::setTitle(const QString &title)
{
	Window *window(qobject_cast<Window*>(sender()));

	if (window)
	{
		const int row(findRow(window->getIdentifier()));

		if (row >= 0)
		{
			m_model->setData(m_model->index(row, 0), title, Qt::DisplayRole);
		}
	}
}

void TabSwitcherWidget::setIcon(const QIcon &icon)
{
	Window *window(qobject_cast<Window*>(sender()));

	if (window)
	{
		const int row(findRow(window->getIdentifier()));

		if (row >= 0)
		{
			m_model->setData(m_model->index(row, 0), icon, Qt::DecorationRole);
		}
	}
}

void TabSwitcherWidget::setLoadingState(WebWidget::LoadingState state)
{
	Window *window(qobject_cast<Window*>(sender()));

	if (window)
	{
		const int row(findRow(window->getIdentifier()));

		if (row >= 0)
		{
			QColor color(palette().color(QPalette::Text));

			if (state == WebWidget::DelayedLoadingState)
			{
				color.setAlpha(150);
			}

			m_model->setData(m_model->index(row, 0), color, Qt::TextColorRole);
		}
	}
}

QStandardItem* TabSwitcherWidget::createRow(Window *window) const
{
	QColor color(palette().color(QPalette::Text));

	if (window->getLoadingState() == WebWidget::DelayedLoadingState)
	{
		color.setAlpha(150);
	}

	QStandardItem* item(new QStandardItem(window->getIcon(), window->getTitle()));
	item->setData(color, Qt::TextColorRole);
	item->setData(window->getIdentifier(), Qt::UserRole);
	item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

	connect(window, SIGNAL(titleChanged(QString)), this, SLOT(setTitle(QString)));
	connect(window, SIGNAL(iconChanged(QIcon)), this, SLOT(setIcon(QIcon)));
	connect(window, SIGNAL(loadingStateChanged(WebWidget::LoadingState)), this, SLOT(setLoadingState(WebWidget::LoadingState)));

	return item;
}

TabSwitcherWidget::SwitcherReason TabSwitcherWidget::getReason() const
{
	return m_reason;
}

int TabSwitcherWidget::findRow(quint64 identifier) const
{
	for (int i = 0; i < m_model->rowCount(); ++i)
	{
		if (m_model->index(i, 0).data(Qt::UserRole).toULongLong() == identifier)
		{
			return i;
		}
	}

	return -1;
}

bool TabSwitcherWidget::eventFilter(QObject *object, QEvent *event)
{
	if (object == m_tabsView->viewport() && event->type() == QEvent::MouseButtonPress)
	{
		QMouseEvent *mouseEvent(static_cast<QMouseEvent*>(event));

		if (mouseEvent && mouseEvent->button() == Qt::MiddleButton)
		{
			const QModelIndex index(m_tabsView->indexAt(mouseEvent->pos()));

			if (index.isValid())
			{
				ActionsManager::triggerAction(ActionsManager::CloseTabAction, parentWidget(), QVariantMap({{QLatin1String("window"), index.data(Qt::UserRole).toULongLong()}}));
			}

			return true;
		}
	}

	return QObject::eventFilter(object, event);
}

}
