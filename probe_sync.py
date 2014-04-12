# -*- coding: utf-8 -*-
"""
    autogarten.probe_sync
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    Object that defines a sync request from a probe

    :license: MIT, see LICENSE for more details.
"""

class ProbeSync(object):
    """ Represents a probe sync request

    """

    def __init__ (self, req_data):
        for key in req_data:
            setattr(self, key, req_data[key])

    def is_valid(self):
        return hasattr(self, "probe_id") and\
            hasattr(self, "connection_attempts")