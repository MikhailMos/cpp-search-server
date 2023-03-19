#pragma once

#include <vector>

template <typename Iterator>
class IteratorRange {
public:
    explicit IteratorRange(Iterator range_begin, Iterator range_end)
        : begin_(range_begin), end_(range_end), size_(distance(range_begin, range_end))
    {}

    Iterator begin() const {
        return begin_;
    }

    Iterator end() const {
        return end_;
    }

    size_t size() const {
        return size_;
    }
private:
    Iterator begin_;
    Iterator end_;
    size_t size_;
};

template <typename Iterator>
class Paginator {
public:
    explicit Paginator(Iterator range_begin, Iterator range_end, size_t page_size)
        : pages_()
    {

        size_t count_items = distance(range_begin, range_end);
        int count_of_pages = count_items / page_size;

        // добавим страницу, которая будет не полной
        if (count_items % page_size != 0) {
            ++count_of_pages;
        }

        for (int i = 0; i < count_of_pages; ++i) {
            // если последняя страница, то заполним ее тем, что осталось
            if (i == count_of_pages - 1) {
                AddPage(range_begin, range_end);
                continue;
            }

            AddPage(range_begin, range_begin + page_size);
            advance(range_begin, page_size);
        }

    }

    typename std::vector<IteratorRange<Iterator>>::const_iterator begin() const {
        return pages_.begin();
    }

    typename std::vector<IteratorRange<Iterator>>::const_iterator end() const {
        return pages_.end();
    }

    size_t size() const {
        return pages_.size();
    }

private:
    std::vector<IteratorRange<Iterator>> pages_;

    void AddPage(Iterator range_begin, Iterator range_end) {
        IteratorRange<Iterator> page(range_begin, range_end);
        pages_.push_back(page);
    }
};

template <typename Container>
auto Paginate(const Container& c, size_t page_size) {
    return Paginator(begin(c), end(c), page_size);
}