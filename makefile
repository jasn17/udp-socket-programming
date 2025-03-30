# Jason  Lin
# Dr. D
# CPSC 3600
# Homework 2
# Makefile for udpchat

CC = gcc
CFLAGS = -Wall -std=c11
TARGET = udpchat

all: $(TARGET)

$(TARGET): udpchat.c
	$(CC) $(CFLAGS) -o $(TARGET) udpchat.c

clean:
	rm -f $(TARGET)

