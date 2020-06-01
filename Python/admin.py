# -*- coding: utf-8 -*-
from __future__ import unicode_literals

from django.contrib import admin
from .models import ServerDeploy, ServerDeployLog, TestProcess, Test2

# Register your models here.
admin.site.register({ServerDeploy, ServerDeployLog, TestProcess, Test2})

class CityAdmin(admin.ModelAdmin):
    icon = '<i class="fa fa-building"></i>'
