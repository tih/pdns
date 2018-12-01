Upgrade Guide
=============

1.3.2 to 1.3.3
--------------

When upgrading from a package before 1.3.3, on CentOS 6 and RHEL 6, dnsdist will be stopped instead of restarted.

1.2.x to 1.3.x
--------------

In version 1.3.0, these things have changed.

The :ref:`Console` has an ACL now, which is set to ``{"127.0.0.0/8", "::1/128"}`` by default.
Add the appropriate :func:`setConsoleACL` and :func:`addConsoleACL` statements to the configuration file.

The ``--daemon`` option is removed from the :program:`dnsdist` binary, meaning that :program:`dnsdist` will not fork to the background anymore.
Hence, it can only be run on the foreground or under a supervisor like systemd, supervisord and ``daemon(8)``.

Due to changes in the architecture of :program:`dnsdist`, several of the shortcut rules have been removed after deprecating them in 1.2.0.
All removed functions have their equivalent :func:`addAction` listed.
Please check the configuration for these statements (or use ``dnsdist --check-config``) and update where needed.
This removal affects these functions:

- :func:`addAnyTCRule`
- :func:`addDelay`
- :func:`addDisableValidationRule`
- :func:`addDomainBlock`
- :func:`addDomainCNAMESpoof`
- :func:`addDomainSpoof`
- :func:`addNoRecurseRule`
- :func:`addPoolRule`
- :func:`addQPSLimit`
- :func:`addQPSPoolRule`

1.1.0 to 1.2.0
--------------

In 1.2.0, several configuration options have been changed:

As the amount of possible settings for listen sockets is growing, all listen-related options must now be passed as a table as the second argument to both :func:`addLocal` and :func:`setLocal`.
See the function's reference for more information.

The ``BlockFilter`` function is removed, as :func:`addRule` combined with a :func:`DropAction` can do the same.
