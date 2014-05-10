# -*- coding: utf-8 -*-
"""
    autogarten.date_util
    ~~~~~~~~~~~~~~~~~~~~~~~~ 

    This module provides various utility functions for working with
    dates.  (Copied from https://github.com/lbracken/news_data)

    :license: MIT, see LICENSE for more details.
"""

import calendar
from datetime import datetime


def get_days_in_month(yyyy, mm):
    """ Returns the number of days in the given month

    """
    return calendar.monthrange(yyyy, mm)[1]


def get_next_month(date_time):
    if date_time.month == 12:
        return datetime(date_time.year + 1, 1, 1)
    else:
        return datetime(date_time.year, date_time.month + 1, 1)


def get_midnight(date_time):
    """ Returns a datetime of midnight (start) of the given day

    """
    return datetime(date_time.year, date_time.month, date_time.day)


def get_timestamp(date_time):
    """ Return the UTC timestamp for the given date

    """
    return int(date_time.strftime("%s"))


def get_current_timestamp():
    """ Return the current UTC timestamp

    """
    return get_timestamp(datetime.now())


def pad_month_day_value(to_pad):
    """ Pads the given month or day value with a preceding '0' if
        needed. For example, turns 2 into '02.  Returned result will
        always be a string
    """
    return str(to_pad if to_pad > 9 else "0%d" % to_pad)


def is_same_day(day_1, day_2):
    """ Return true if the two given dates are the same day

    """
    return is_same_month(day_1, day_2) and (day_1.day == day_2.day)


def is_same_month(month_1, month_2):
    """ Return true if the two given dates are in the same month

    """
    return is_same_year(month_1, month_2) and (month_1.month == month_2.month)


def is_same_year(year_1, year_2):
    """ Return true if the two given dates are in the same year

    """
    if (year_1 == None or year_2 == None):
        return False

    return (year_1.year == year_2.year)    