# -*- coding: utf-8 -*-
# Generated by Django 1.11.16 on 2019-10-18 18:46
from __future__ import unicode_literals

from django.db import migrations, models


class Migration(migrations.Migration):

    dependencies = [
        ('ews', '0001_initial'),
    ]

    operations = [
        migrations.AddField(
            model_name='build',
            name='retried',
            field=models.BooleanField(default=False),
        ),
    ]
