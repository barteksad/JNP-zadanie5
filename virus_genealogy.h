#ifndef VIRUS_GENEALOGY_H
#define VIRUS_GENEALOGY_H

#include <exception>
#include <unordered_map>
#include <vector>
#include <list>
#include <memory>
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
    using id_type = typename Virus::id_type;
    struct VirusNode;

    id_type stem_id;


public:
    struct children_iterator;

    VirusGenealogy(id_type const &stem_id);
    id_type get_stem_id() const;
    children_iterator get_children_begin(id_type const &id) const throw();
    children_iterator get_children_end(id_type const &id) const throw();
    std::vector<id_type> get_parents(id_type const &id) const throw();
    bool exists(id_type const &id) const;
    const Virus &operator[](id_type const &id) const throw();
    void create(id_type const &id, id_type const &parent_id) throw();
    void create(id_type const &id, std::vector<id_type> const &parent_ids) throw();
    void connect(id_type const &child_id, id_type const &parent_id) throw();
    void remove(id_type const &id) throw();
};

template <typename Virus>
VirusGenealogy<Virus>::VirusGenealogy(id_type const &stem_id)
    : stem_id(stem_id) {}

template <typename Virus>
struct VirusGenealogy<Virus>::VirusNode
{
    Virus virus;
    std::list<std::shared_ptr<Virus>> parents;
    std::list<std::shared_ptr<Virus>> childrens;

    VirusNode(Virus::id_type new_virus_id)
        : virus(new_virus_id) {}
};

#endif