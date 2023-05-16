#pragma once

#include "macros.h"

// ���� ���������, ��� ��������� ������� ��������� ����-����� ��� ���������� ����������
void TestExcludeStopWordsFromAddedDocumentContent();

// ���� ���������, ��� ��������� ������� ��������� �����-����� �� ���������� �������
void TestExcludeMinusWordsFromQuery();

// ���� �� �������� ������������� ����������� ��������� � ���������� �������
void TestMatchDocument();

// ���� �� ���������� �� ������������� �������� ����������
void TestByRelevance();

// ���� �� ��������
void TestByRating();

// ���� �� ���������� ���������� � �������������� ���������
void TestFilterByPredicate();

void TestSearchByStatusDocuments();

// ������� TestSearchServer �������� ������ ����� ��� ������� ������
void TestSearchServer();