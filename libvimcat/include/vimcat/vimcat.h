#pragma once

#ifndef VIMCAT_API
#ifdef __GNUC__
#define VIMCAT_API /* nothing */
#elif defined(_MSC_VER)
#define VIMCAT_API __declspec(dllimport)
#else
#define VIMCAT_API /* nothing */
#endif
#endif

#include <vimcat/debug.h>
#include <vimcat/have_vim.h>
#include <vimcat/read.h>
#include <vimcat/version.h>
