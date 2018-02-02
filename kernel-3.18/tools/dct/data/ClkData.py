#! /usr/bin/python
# -*- coding: utf-8 -*-

class ClkData:
    _varList = ['DISABLE', 'SW_CONTROL', 'HW_CONTROL']
    _count = 0

    def __init__(self):
        self.__varName = ''
        self.__current = ''
        self.__curList = []

    def set_defVarName(self, idx):
        self.__varName = self._varList[idx]

    def set_varName(self, name):
        self.__varName = name

    def set_defCurrent(self, idx):
        self.__current = self.__curList[idx]

    def set_current(self, current):
        self.__current = current

    def get_varName(self):
        return self.__varName

    def get_current(self):
        return self.__current

    def set_curList(self, cur_list):
        self.__curList = cur_list

    def get_curList(self):
        return self.__curList
