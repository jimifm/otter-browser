/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2017 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#ifndef OTTER_MIGRATOR_H
#define OTTER_MIGRATOR_H

#include <QtCore/QHash>
#include <QtCore/QObject>

namespace Otter
{

class Migration
{
public:
	Migration();
	virtual ~Migration();

	virtual void createBackup() const;
	virtual void migrate() const;
	virtual QString getName() const;
	virtual QString getTitle() const;
	virtual bool canMigrate() const;

protected:
	static QString createBackupPath(const QString &sourcePath);
};

class Migrator : public QObject
{

public:
	enum MigrationFlag
	{
		NoMigration = 0,
		IgnoreMigration = 1,
		ProceedMigration = 2,
		WithBackupMigration = 4
	};

	Q_DECLARE_FLAGS(MigrationFlags, MigrationFlag)

	explicit Migrator(QObject *parent = nullptr);

	void run();

protected:
	MigrationFlags checkMigrationStatus(const Migration *migration) const;
};

}

Q_DECLARE_OPERATORS_FOR_FLAGS(Otter::Migrator::MigrationFlags)

#endif
