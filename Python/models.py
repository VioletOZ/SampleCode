# -*- coding: utf-8 -*-
from __future__ import unicode_literals

from django.db import models
from viewflow.models import Process

# import sys
# reload(sys)
# sys.setdefaultencoding('utf-8')

# Create your models here.
class TestProcess(Process):
    text = models.CharField(max_length=150)
    approved = models.BooleanField(default=False)

class ServerDeploy(models.Model):
    server_deploy_text = models.CharField(max_length=200)
    pub_data = models.DateTimeField('date published')

    s = '서버배포'
    server_deploy_text.verbose_name=str(s)

    created_at = models.DateTimeField(auto_now_add=True)
    updated_at = models.DateTimeField(auto_now=True)

class ServerDeployLog(models.Model):
    ServerDeployLog = models.ForeignKey(ServerDeploy, on_delete=models.CASCADE)
    deployer = models.CharField(max_length=200)
    number = models.IntegerField(default=0)

    ServerDeployLog.verbose_name = str(('서버로그'))
    deployer.verbose_name = str(('배포자'))
    number.verbose_name = str(('일련번호'))

    updated_at = models.DateTimeField(auto_now=True)

class Test2(models.Model):
    text = models.CharField(max_length=200)
    text.verbose_nam=str(('테스트'))