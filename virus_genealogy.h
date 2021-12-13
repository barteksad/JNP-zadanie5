#ifndef VIRUS_GENEALOGY_H
#define VIRUS_GENEALOGY_H

#include <exception>
#include <unordered_map>
#include <vector>
#include <memory>
// #include <iostream>

class VirusNotFound : public std::exception
{
    // TODO : dziedziczenie po odpowiednim wyjątku
};

class VirusAlreadyCreated : public std::exception
{
    // TODO : dziedziczenie po odpowiednim wyjątku
};

class TriedToRemoveStemVirus : public std::exception
{
    // TODO : dziedziczenie po odpowiednim wyjątku
};

template <typename Virus>
class VirusGenealogy
{
private:
    class VirusNode;
    class InsertVirusGuard;

    using id_type = typename Virus::id_type;
    using virus_map_shared = std::unordered_map<id_type, std::shared_ptr<VirusNode>>;
    using virus_map_weak = std::unordered_map<id_type, std::weak_ptr<VirusNode>>;

    virus_map_shared::iterator find_node(id_type const &id);

    id_type stem_id;
    virus_map_shared nodes;

public:
    struct children_iterator;

    VirusGenealogy(id_type const &_stem_id);
    id_type get_stem_id() const;
    children_iterator get_children_begin(id_type const &id) const;
    children_iterator get_children_end(id_type const &id) const;
    std::vector<id_type> get_parents(id_type const &id) const;
    bool exists(id_type const &id) const;
    const Virus &operator[](id_type const &id) const;
    void create(id_type const &id, id_type const &parent_id);
    void create(id_type const &id, std::vector<id_type> const &parent_ids);
    void connect(id_type const &child_id, id_type const &parent_id);
    void remove(id_type const &id);
};

template <typename Virus>
VirusGenealogy<Virus>::VirusGenealogy(id_type const &_stem_id)
    : stem_id(_stem_id), nodes({{_stem_id, std::make_shared<VirusNode>(_stem_id)}}) {}

template <typename Virus>
class VirusGenealogy<Virus>::VirusNode
{
private:
    Virus virus;
    virus_map_weak parents;
    virus_map_weak children;

public:
    VirusNode(Virus::id_type new_virus_id)
        : virus(new_virus_id) {}

    // ~VirusNode() {
    //     std::cout<<" VirusNode dest\n";
    // }

    virus_map_weak &get_parents() noexcept
    {
        return parents;
    }

    virus_map_weak &get_children() noexcept
    {
        return children;
    }

    Virus &get_virus()
    {
        return virus;
    }

    id_type get_id() const
    {
        return virus.get_id();
    }
};

template <typename Virus>
class VirusGenealogy<Virus>::InsertVirusGuard
{
private:
    bool rollback;
    bool rollback_at_construct_time;
    virus_map_weak& inser_place;
    virus_map_weak::iterator it;

public:
    InsertVirusGuard(virus_map_weak &_inser_place, std::shared_ptr<VirusNode> virus_node)
        : rollback(false), inser_place(_inser_place)
    {
        rollback_at_construct_time = true;
        bool present;

        std::weak_ptr node = virus_node;

        std::tie(it, present) = inser_place.insert({virus_node->get_id(), node});
        if (!present)
        {
            rollback_at_construct_time = false;
            throw new VirusAlreadyCreated();
        }

        rollback = true;
    }

    ~InsertVirusGuard() noexcept
    {
        if (rollback && rollback_at_construct_time)
            inser_place.erase(it);
    }

    void dropRollback() noexcept
    {
        rollback = false;
    }
};

template <typename Virus>
VirusGenealogy<Virus>::virus_map_shared::iterator VirusGenealogy<Virus>::find_node(id_type const &id)
{
    typename virus_map_shared::iterator it = nodes.find(id);
    if (it == nodes.end())
        throw new VirusNotFound();
    else
        return it;
}

template <typename Virus>
const Virus &VirusGenealogy<Virus>::operator[](id_type const &id) const
{

    typename virus_map_shared::iterator it = find_node(id);
    return it->second.get_virus();
}

template <typename Virus>
VirusGenealogy<Virus>::id_type VirusGenealogy<Virus>::get_stem_id() const
{
    return stem_id;
}

// VirusAlreadyCreated VirusNotFound
template <typename Virus>
void VirusGenealogy<Virus>::create(id_type const &id, std::vector<id_type> const &parent_ids)
{
    std::shared_ptr<VirusNode> new_node = std::make_shared<VirusNode>(id);

    std::vector<std::unique_ptr<InsertVirusGuard>> insertGuards;

    for (id_type parent_id : parent_ids)
    {
        std::shared_ptr<VirusNode> parent = find_node(parent_id)->second;

        insertGuards.push_back(std::make_unique<InsertVirusGuard>(parent->get_children(), new_node));
        insertGuards.push_back(std::make_unique<InsertVirusGuard>(new_node->get_parents(), parent));
    }

    nodes.insert({id, new_node});
    // insertGuards.push_back(std::make_unique<InsertVirusGuard>(nodes, new_node));

    for(auto& guard : insertGuards)
        guard->dropRollback();
}

#endif