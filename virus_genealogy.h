#ifndef VIRUS_GENEALOGY_H
#define VIRUS_GENEALOGY_H

#include <exception>
#include <map>
#include <vector>
#include <memory>
#include <queue>
#include <set>

class VirusNotFound : public std::runtime_error {
public:
    VirusNotFound() : runtime_error("VirusNotFound") {}
};

class VirusAlreadyCreated : public std::runtime_error {
public:
    VirusAlreadyCreated() : runtime_error("VirusAlreadyCreated") {}
};

class TriedToRemoveStemVirus : public std::invalid_argument {
public:
    TriedToRemoveStemVirus() : invalid_argument("TriedToRemoveStemVirus") {}
};

template<typename Virus>
class VirusGenealogy {
private:
    class VirusNode;
    class InsertVirusGuard;
    class RemoveVirusGuard;

    using id_type = typename Virus::id_type;
    using virus_map_shared = std::map<id_type, std::shared_ptr<VirusNode>>;
    using virus_map_weak = std::map<id_type, std::weak_ptr<VirusNode>>;

    typename virus_map_shared::iterator find_node(id_type const &id) {
        typename virus_map_shared::iterator it = nodes.find(id);
        if (it == nodes.end())
            throw VirusNotFound();
        else
            return it;
    }

    typename virus_map_shared::const_iterator find_node(id_type const &id) const {
        typename virus_map_shared::const_iterator it = nodes.find(id);
        if (it == nodes.end())
            throw VirusNotFound();
        else
            return it;
    }

    id_type stem_id;
    virus_map_shared nodes;

public:
    // Konstruktory
    VirusGenealogy(id_type const &_stem_id)
            : stem_id(_stem_id)
            ,  nodes({{_stem_id, std::make_shared<VirusNode>(_stem_id)}}) {}

    VirusGenealogy(const VirusGenealogy &other) = delete;
    // Koniec konstruktorów

    // Operatory
    VirusGenealogy &operator=(const VirusGenealogy &other) = delete;
    const Virus &operator[](id_type const &id) const {
        typename virus_map_shared::const_iterator it = find_node(id);
        return it->second->get_virus();
    }
    // Koniec operatorów


    // Początek metod publicznych
    id_type get_stem_id() const {
        return stem_id;
    }

    bool exists(id_type const &id) const {
        return nodes.contains(id);
    }

    void create(id_type const &id, id_type const &parent_id) {
        std::vector<id_type> tmp({parent_id});
        create(id, tmp);
    }

    std::vector<id_type> get_parents(id_type const &id) const {
        const auto it = find_node(id);
        std::vector<typename VirusGenealogy<Virus>::id_type> parents;

        for (auto p : it->second->get_parents())
            parents.push_back(p.second.lock()->get_id());

        return parents;
    }

    void create(id_type const &id, std::vector<id_type> const &parent_ids) {
        if(parent_ids.empty())
            return;
        if (nodes.find(id) != nodes.end())
            throw VirusAlreadyCreated();

        std::shared_ptr<VirusNode> new_node = std::make_shared<VirusNode>(id);
        std::vector<std::shared_ptr<InsertVirusGuard>> insertGuards;
        std::set<id_type> s (parent_ids.begin(), parent_ids.end());

        for (id_type parent_id : s) {
            std::shared_ptr<VirusNode> parent = find_node(parent_id)->second;

            insertGuards.push_back(std::make_shared<InsertVirusGuard>(
                    parent->get_children(), new_node));
            insertGuards.push_back(std::make_shared<InsertVirusGuard>(
                    new_node->get_parents(), parent));
        }
        nodes.insert({id, new_node});

        for (auto &guard : insertGuards)
            guard->dropRollback();
    }

    void connect(id_type const &child_id, id_type const &parent_id) {
        std::shared_ptr<VirusNode> child = find_node(child_id)->second;
        std::shared_ptr<VirusNode> parent = find_node(parent_id)->second;

        if (child_id == stem_id)
            return;

        std::unique_ptr<InsertVirusGuard> insertGuards1 =
                std::make_unique<InsertVirusGuard>(
                        parent->get_children(), child, false);
        std::unique_ptr<InsertVirusGuard> insertGuards2 =
                std::make_unique<InsertVirusGuard>(
                        child->get_parents(), parent, false);

        insertGuards1->dropRollback();
        insertGuards2->dropRollback();
    }

    void remove(id_type const &id);
    // Koniec metod publicznych


    // Początek Children Iterator
    class children_iterator;

    children_iterator get_children_begin(id_type const &id) const {
        const auto it = find_node(id);
        return children_iterator(it->second->get_children().begin());
    }

    children_iterator get_children_end(id_type const &id) const {
        const auto it = find_node(id);
        return children_iterator(it->second->get_children().end());
    }
    // Koniec Children Iterator
};

// Implementacja metody remove, klasy VirusGeneaology
template <typename Virus>
void VirusGenealogy<Virus>::remove(id_type const &id) {
    if (stem_id == id)
        throw TriedToRemoveStemVirus();

    auto remove_virus_it = find_node(id);
    if (remove_virus_it == nodes.end())
        throw VirusNotFound();

    std::vector<decltype(nodes.begin())> to_erase({remove_virus_it});
    std::set<id_type> to_erase_ids({id});
    std::vector<std::shared_ptr<RemoveVirusGuard>> removeGuards;

    for (auto &[parent_id, parent] : remove_virus_it->second->get_parents())
        removeGuards.push_back(std::make_shared<RemoveVirusGuard>(
                                    parent.lock()->get_children(), id));

    // <parent_id, child_pointer>
    std::queue<std::pair<id_type, std::shared_ptr<VirusNode>>> bfs;
    for (auto &[child_id, child] : remove_virus_it->second->get_children())
        bfs.push({id, child.lock()});

    while (!bfs.empty()) {
        auto &[parent_id, current] = bfs.front();
        bfs.pop();

        bool all_parents_to_delete = std::all_of(
                current->get_parents().begin(),
                current->get_parents().end(),
                [&](auto it) {
                    return to_erase_ids.find(it.second.lock()->get_id())
                                                    != to_erase_ids.end();}
                                                    );

        if (all_parents_to_delete) {
            to_erase.push_back(find_node(current->get_id()));
            to_erase_ids.insert(current->get_id());

            for (auto &[child_id, child]
                    : current->get_children())
                bfs.push({current->get_id(), child.lock()});
        }
        else {
            removeGuards.push_back(std::make_shared<RemoveVirusGuard>(
                    current->get_parents(), parent_id));
        }
    }

    for (auto &g : removeGuards)
        g->dropRollback();

    removeGuards.clear();

    for (auto &it : to_erase)
        nodes.erase(it);
}

// Początek implementacji klas prywatnych

template<typename Virus>
class VirusGenealogy<Virus>::VirusNode {
private:
    Virus virus;
    virus_map_weak parents;
    virus_map_weak children;
public:
    VirusNode(typename Virus::id_type new_virus_id) : virus(new_virus_id) {}

    virus_map_weak &get_parents() noexcept { return parents; }

    virus_map_weak &get_children() noexcept { return children; }

    Virus &get_virus() noexcept { return virus; }

    id_type get_id() const { return virus.get_id(); }
};

// Insert Guards
template<typename Virus>
class VirusGenealogy<Virus>::InsertVirusGuard {
private:
    bool rollback;
    bool rollback_at_construct_time;
    virus_map_weak &insert_place;
    typename virus_map_weak::iterator it;

public:
    InsertVirusGuard(virus_map_weak &_insert_place,
                     std::weak_ptr<VirusNode> virus_node,
                     [[maybe_unused]] bool _must_be_new = true)
            : rollback(false)
            , insert_place(_insert_place) {
        rollback_at_construct_time = true;
        bool inserted;
        std::tie(it, inserted) = insert_place.insert(
                {virus_node.lock()->get_id(), virus_node});

        if (!inserted && _must_be_new) {
            rollback_at_construct_time = false;
            throw VirusAlreadyCreated();
        } else if (!inserted) {
            rollback_at_construct_time = false;
        }
        rollback = true;
    }

    ~InsertVirusGuard() noexcept {
        if (rollback && rollback_at_construct_time)
            insert_place.erase(it);
    }

    void dropRollback() noexcept { rollback = false;}

    InsertVirusGuard(InsertVirusGuard const &) = delete;
    InsertVirusGuard &operator=(InsertVirusGuard const &) = delete;
};

template<typename Virus>
class VirusGenealogy<Virus>::RemoveVirusGuard {
private:
    bool rollback;
    virus_map_weak &erase_place;
    typename virus_map_weak::iterator it;

public:
    RemoveVirusGuard(virus_map_weak &_erase_place, const id_type id)
            : rollback(true), erase_place(_erase_place) {
        it = _erase_place.find(id);
    }

    ~RemoveVirusGuard() noexcept {
        if (!rollback)
            erase_place.erase(it);
    }

    void dropRollback() noexcept { rollback = false;}
};

// Koniec implementacji klas prywatnych

// Implementacja klasy children_iterator
template <typename Virus>
class VirusGenealogy<Virus>::children_iterator {
private:
    typename virus_map_weak::const_iterator ptr;
public:
    using iterator_category = std::bidirectional_iterator_tag;
    using value_type = Virus;
    using difference_type = ptrdiff_t;
    using pointer = const value_type *;
    using reference = const Virus &;

    children_iterator() = default;
    children_iterator(const children_iterator &other) = default;
    children_iterator(children_iterator &&other) noexcept = default;
    children_iterator(decltype(ptr) &&iter) : ptr(iter) {};

    children_iterator &operator=(const children_iterator &other) = default;
    children_iterator &operator=(children_iterator &&other) noexcept = default;

    reference operator*() const {
        return ptr->second.lock()->get_virus();
    }

    pointer operator->() const {
        return &(ptr->second.lock()->get_virus());
    }

    children_iterator &operator++() {
        ptr++;
        return *this;
    }

    children_iterator operator++(int) {
        children_iterator result(*this);
        operator++();
        return result;
    }

    children_iterator &operator--() {
        ptr--;
        return *this;
    }
    children_iterator operator--(int) {
        children_iterator result(*this);
        operator--();
        return result;
    }

    friend bool operator==(children_iterator const &a,
                           children_iterator const &b) {
        return a.ptr == b.ptr;
    }
    friend bool operator!=(children_iterator const &a,
                           children_iterator const &b) {
        return !(a == b);
    }
};

#endif //VIRUS_GENEALOGY_H