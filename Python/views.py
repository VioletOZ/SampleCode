# -*- coding: utf-8 -*-
from __future__ import unicode_literals

from django.shortcuts import render
from django.http import HttpResponse

# Create your views here.

def test(request):
    return render(request, './admin/apps/index.html', {})


def time(request):
    return render(request, './admin/apps/time.html', {})

def documents(request):
    pass