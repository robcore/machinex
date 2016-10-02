#!/bin/bash
patch -p1 -R < "0092-revert-those-im-pretty-sure-we-did-this-a-long-time-.patch"
patch -p1 -R < "0091-lockdep-check-that-no-locks-held-at-freeze-time.patch"
patch -p1 -R < "0090-freezer-add-unsafe-versions-of-freezable-helpers-for.patch"
patch -p1 -R < "0089-freezer-added-unsafe-versions-of-freezable-helpers-f.patch"
patch -p1 -R < "0088-freezer-define-try_to_freeze_nowarn.patch"
