#!/usr/bin/env bash

uv sync
uv run python manage.py migrate
npm ci
npm run build
