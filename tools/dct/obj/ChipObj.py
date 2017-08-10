#! /usr/bin/python
# -*- coding: utf-8 -*-

import os, sys
import xml.dom.minidom

from GpioObj import GpioObj
from EintObj import EintObj
from AdcObj import AdcObj
from ClkObj import ClkObj
from ClkObj import ClkObj_Everest
from ClkObj import ClkObj_Olympus
from ClkObj import ClkObj_Rushmore
from I2cObj import I2cObj
from PmicObj import PmicObj
from Md1EintObj import Md1EintObj
from PowerObj import PowerObj
from KpdObj import KpdObj
from ModuleObj import ModuleObj

from utility.util import log
from utility.util import LogLevel

para_map = {'adc':['adc_h', 'adc_dtsi'],\
            'clk':['clk_buf_h', 'clk_buf_dtsi'],\
            'eint':['eint_h', 'eint_dtsi'],\
            'gpio':['gpio_usage_h', 'gpio_boot_h', 'gpio_dtsi', 'scp_gpio_usage_h', 'pinfunc_h', \
                    'pinctrl_h', 'gpio_usage_mapping_dtsi', 'gpio_usage_mapping_dtsi'],\
            'i2c':['i2c_h', 'i2c_dtsi'],\
            'md1_eint':['md1_eint_h', 'md1_eint_dtsi'],\
            'kpd':['kpd_h', 'kpd_dtsi'],\
            'pmic':['pmic_drv_h', 'pmic_drv_c', 'pmic_dtsi'],\
            'power':['power_h']}

class ChipObj:
    def __init__(self, path, dest):
        self.__path = path
        ModuleObj.set_genPath(dest)
        self.__objs = {}

        self.init_objs()

    def init_objs(self):
        self.__objs['adc'] = AdcObj()
        self.__objs['clk'] = ClkObj()
        self.__objs["gpio"] = GpioObj()
        # eint obj need gpio data
        self.__objs["eint"] = EintObj(self.__objs['gpio'])
        self.__objs["i2c"] = I2cObj()
        self.__objs["md1_eint"] = Md1EintObj()

        self.__objs["pmic"] = PmicObj()
        self.__objs["power"] = PowerObj()
        self.__objs["kpd"] = KpdObj()

    def replace_obj(self, tag, obj):
        if not tag in self.__objs.keys():
            return False

        self.__objs[tag] = obj

    def append_obj(self, tag, obj):
        if tag in self.__objs.keys():
            return False

        self.__objs[tag] = obj

    @staticmethod
    def get_chipId(path):
        if not os.path.exists(path):
            msg = '%s is not a available path!' %(path)
            log(LogLevel.error, msg)
            return False
        data = xml.dom.minidom.parse(path)
        root = data.documentElement
        # get 'general' node
        node = root.getElementsByTagName('general')
        return node[0].getAttribute('chip')

    def parse(self):
        if not os.path.exists(self.__path):
            msg = '%s is not a available path!' %(self.__path)
            log(LogLevel.error, msg)
            return False

        data = xml.dom.minidom.parse(self.__path)

        root = data.documentElement
        # get 'general' node
        node = root.getElementsByTagName('general')
        # get chip name and project name
        ModuleObj.set_chipId(node[0].getAttribute('chip'))

        msg = 'Chip ID : %s' %(node[0].getAttribute('chip'))
        log(LogLevel.info, msg)
        msg = 'Project Info: %s' %(node[0].getElementsByTagName('proj')[0].childNodes[0].nodeValue)
        log(LogLevel.info, msg)

        # initialize the objects mapping table
        self.init_objs()
        # get module nodes from DWS file
        nodes = node[0].getElementsByTagName('module')

        for node in nodes:
            tag = node.getAttribute('name')
            obj = self.create_obj(tag)
            if obj == None:
                msg = 'can not find %s node in DWS!' %(tag)
                log(LogLevel.error, msg)
                return False
            obj.parse(node)

        return True

    def generate(self, paras):
        if len(paras) == 0:
            for obj in self.__objs.values():
                obj.gen_files()

            self.gen_custDtsi()
        else:
            self.gen_spec(paras)

        return True

    def create_obj(self, tag):
        obj = None
        if tag in self.__objs.keys():
            obj = self.__objs[tag]

        return obj


    def gen_spec(self, paras):
        if cmp(paras[0], 'cust_dtsi') == 0:
            self.gen_custDtsi()
            return True

        for para in paras:
            idx = 0
            name = ''
            if para.strip() != '':
                for value in para_map.values():
                    if para in value:
                        name = para_map.keys()[idx]
                        break
                    idx += 1

            if name != '':
                log(LogLevel.info, 'Start to generate %s file...' %(para))
                obj = self.__objs[name]
                obj.gen_spec(para)
                log(LogLevel.info, 'Generate %s file successfully!' %(para))
            else:
                log(LogLevel.error, '%s can not be recognized!' %(para))
                sys.exit(-1)

        return True

    def gen_custDtsi(self):
        log(LogLevel.info, 'Start to generate cust_dtsi file...')
        fp = open(os.path.join(ModuleObj.get_genPath(), 'cust.dtsi'), 'w')
        gen_str = ModuleObj.writeComment()

        sorted_list = sorted(self.__objs.keys())
        for tag in sorted_list:
            if cmp(tag, 'gpio') == 0:
                gpioObj = self.create_obj(tag)
                gen_str += ModuleObj.writeHeader(gpioObj.get_dtsiFileName())
                gen_str += gpioObj.fill_mapping_dtsiFile()
            else:
                obj = self.create_obj(tag)
                gen_str += ModuleObj.writeHeader(obj.get_dtsiFileName())
                gen_str += obj.fill_dtsiFile()

            gen_str += '''\n\n'''

        fp.write(gen_str)
        fp.close()
        log(LogLevel.info, 'Generate cust_dtsi file successfully!')


class Everest(ChipObj):
    def __init__(self, dws_path, gen_path):
        ChipObj.__init__(self, dws_path, gen_path)
        self.init_objs()

    def init_objs(self):
        ChipObj.init_objs(self)
        ChipObj.replace_obj(self, 'clk', ClkObj_Everest())

    def parse(self):
        return ChipObj.parse(self)

    def generate(self, paras):
        return ChipObj.generate(self, paras)


class Olympus(ChipObj):
    def __init__(self, dws_path, gen_path):
        ChipObj.__init__(self, dws_path, gen_path)

    def init_objs(self):
        ChipObj.init_objs(self)
        ChipObj.replace_obj(self, 'clk', ClkObj_Olympus())

    def parse(self):
        return ChipObj.parse(self)


    def generate(self, paras):
        return ChipObj.generate(self, paras)


class KiboPlus(ChipObj):
    def __init__(self, dws_path, gen_path):
        ChipObj.__init__(self, dws_path, gen_path)

    def init_objs(self):
        ChipObj.init_objs(self)
        ChipObj.replace_obj(self, 'clk', ClkObj_Olympus())

    def parse(self):
        return ChipObj.parse(self)


    def generate(self, paras):
        return ChipObj.generate(self, paras)


class Rushmore(ChipObj):
    def __init__(self, dws_path, gen_path):
        ChipObj.__init__(self, dws_path, gen_path)

    def init_objs(self):
        ChipObj.init_objs(self)
        ChipObj.replace_obj(self, 'clk', ClkObj_Rushmore())

    def parse(self):
        return ChipObj.parse(self)


    def generate(self, paras):
        return ChipObj.generate(self, paras)




