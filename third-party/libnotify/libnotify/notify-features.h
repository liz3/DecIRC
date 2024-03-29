/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*-
 *
 * Copyright (C) 2010 Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA  02111-1307, USA.
 */

#ifndef __NOTIFY_VERSION_H__
#define __NOTIFY_VERSION_H__

/* compile time version
 */

/**
 * NOTIFY_VERSION_MAJOR:
 *
 * Adwaita major version component (e.g. 1 if the version is 1.2.3).
 */
#define NOTIFY_VERSION_MAJOR    (0)
/**
 * NOTIFY_VERSION_MINOR:
 *
 * Adwaita minor version component (e.g. 2 if the version is 1.2.3).
 */
#define NOTIFY_VERSION_MINOR    (8)
/**
 * NOTIFY_VERSION_MICRO:
 *
 * Adwaita micro version component (e.g. 3 if the version is 1.2.3).
 */
#define NOTIFY_VERSION_MICRO    (2)

/**
 * NOTIFY_CHECK_VERSION:
 * @major: required major version
 * @minor: required minor version
 * @micro: required micro version
 *
 * check whether a version equal to or greater than
 * `major.minor.micro` is present.
 */
#define NOTIFY_CHECK_VERSION(major,minor,micro) \
    (NOTIFY_VERSION_MAJOR > (major) || \
     (NOTIFY_VERSION_MAJOR == (major) && NOTIFY_VERSION_MINOR > (minor)) || \
     (NOTIFY_VERSION_MAJOR == (major) && NOTIFY_VERSION_MINOR == (minor) && \
      NOTIFY_VERSION_MICRO >= (micro)))


#endif /* __NOTIFY_VERSION_H__ */

