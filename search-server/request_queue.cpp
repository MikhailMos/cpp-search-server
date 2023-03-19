#include "request_queue.h"

RequestQueue::RequestQueue(const SearchServer& search_server)
    : requests_()
    , search_server_(search_server)
    , count_requests_without_result(0)
    , curr_time_(0) {
}

// ������� "������" ��� ���� ������� ������, ����� ��������� ���������� ��� ����� ����������
template <typename DocumentPredicate>
std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate) {

    const std::vector<Document> result_docs = search_server_.FindTopDocuments(raw_query, document_predicate);
    RequestQueue::AddRequest(raw_query, static_cast<int>(result_docs.size()));

    return result_docs;
}

std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query, DocumentStatus status) {
    return AddFindRequest(raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
        return document_status == status;
        });
}

std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query) {
    return AddFindRequest(raw_query, DocumentStatus::ACTUAL);
}

int RequestQueue::GetNoResultRequests() const {
    return count_requests_without_result;
}

void RequestQueue::AddRequest(const std::string& raw_query, int count_of_docs) {
    // ����������� �����
    ++curr_time_;

    // ������ ���������� �������, ���� ��� �������
    while (!requests_.empty() && min_in_day_ <= (curr_time_ - requests_.front().time)) {
        // ��������� ���-�� �������� � ������ ��������
        if (requests_.front().count_docs == 0) {
            --count_requests_without_result;
        }
        // ������� ������ ������
        requests_.pop_front();
    }

    // ������� ���������� �������� ��� ����������
    if (count_of_docs == 0) {
        ++count_requests_without_result;
    }

    requests_.push_back({ raw_query, count_of_docs, curr_time_ });

}