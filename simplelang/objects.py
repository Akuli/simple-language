import collections


class Namespace:

    def __init__(self, content_string_for_messages, parent_namespace=None):
        # examples: 'attribute', 'variable'
        self.content_string_for_messages = content_string_for_messages

        if parent_namespace is None:
            parent_values = []
        else:
            parent_values = parent_namespace.values.maps

        # self.values is {name: [value, is_constant]}
        # i know i know, tuples would be better... but they are immutable
        # the {} means that anything defined here is not visible in the
        # parent namespaces
        self.values = collections.ChainMap({}, *parent_values)

        # this can be set to False to prevent add()ing more things
        self.can_add = True

    def add(self, name, initial_value, is_const=True):
        if not self.can_add:
            raise ValueError("cannot create more {}s"
                             .format(self.content_string_for_messages))
        if name in self.values:
            kind = 'constant' if self.values[name][1] else 'non-constant'
            raise ValueError("there's already a %s '%s' %s"
                             % (kind, name, self.content_string_for_messages))
        self.values[name] = [initial_value, is_const]

    def set(self, name, value):
        try:
            if self.values[name][1]:
                raise ValueError("cannot set '%s', it's a constant" % name)
            self.values[name][0] = value
        except KeyError as e:
            raise ValueError("no %s named '%s'"
                             % (self.content_string_for_messages, name)) from e

    def get(self, name):
        try:
            return self.values[name][0]
        except KeyError as e:
            raise ValueError("no %s named '%s'"
                             % (self.content_string_for_messages, name)) from e


def _get_attrs(obj):
    result = set()
    for attribute in obj.attributes.values:
        result.add((attribute, obj.attributes.get(attribute)))
    return result

class Object:

    def __init__(self):
        self.attributes = Namespace('attribute')

    # TODO: provide some way to customize these??
    def __eq__(self, other):
        if not isinstance(other, Object):
            return NotImplemented

        # TODO: is this tooo duck-typingy?
        return _get_attrs(self) == _get_attrs(other)

    def __hash__(self):
        return hash(frozenset(_get_attrs(self)))


class NullClass(Object):

    def __init__(self):
        if 'null' in globals():    # null object already created
            raise TypeError("cannot create more NullClass objects, use the "
                            "existing null object instead")
        super().__init__()
        self.attributes.can_add = False

    def __eq__(self, other):
        return (self is other)

    def __hash__(self):
        return hash(None)   # should be unique enough


null = NullClass()


class String(Object):

    def __init__(self, python_string):
        super().__init__()
        self.python_string = python_string
        self.attributes.can_add = False

    def __repr__(self):
        return '<simplelang String object: %r>' % self.python_string

    # TODO: get rid of the boilerplate :(
    def __eq__(self, other):
        if not isinstance(other, String):
            return NotImplemented
        return self.python_string == other.python_string

    def __hash__(self):
        return hash(self.python_string)


class BuiltinFunction(Object):

    def __init__(self, python_func):
        super().__init__()
        self.python_func = python_func
        self.attributes.can_add = False

    def call(self, args):
        return self.python_func(*args)

    def __eq__(self, other):
        if not isinstance(other, BuiltinFunction):
            return NotImplemented
        return self.python_func == other.python_func

    def __hash__(self):
        return hash(self.python_func)


# to be expanded...
class Array(Object):

    def __init__(self, elements=()):
        super().__init__()
        self.python_list = list(elements)
        self.attributes.can_add = False


class Mapping(Object):

    def __init__(self, elements=None):
        super().__init__()
        self.python_dict = ({} if elements is None else dict(elements))

        self.attributes.add(
            'set', BuiltinFunction(self.python_dict.__setitem__))
        self.attributes.add('get', BuiltinFunction(self.get))
        self.attributes.add(
            'delete', BuiltinFunction(self.python_dict.__delitem__))
        self.attributes.can_add = False

    @classmethod
    def from_pairs(cls, *pairs):
        assert len(pairs) % 2 == 0, "odd number of arguments: " + repr(pairs)

        # idiomatic python: iterate in pairs
        result = {}
        iterator = iter(pairs)
        for key, value in zip(iterator, iterator):
            result[key] = value

        return cls(result)

    def set(self, key, value):
        self.python_dict[key] = value

    # python's dict.get returns None when default is not set and key not
    # found, this one raisese instead
    def get(self, key, default=None):
        try:
            return self.python_dict[key]
        except KeyError as e:
            if default is not None:
                return default
            raise e

    # TODO: keys,values,items equivalents? maybe as iterators or views?


class Code(Object):

    def __init__(self, interpreter, definition_context, ast_statements):
        assert ast_statements is not iter(ast_statements), (
            "ast_statements cannot be an iterator because it may need to be "
            "looped over several times")

        super().__init__()
        self._interp = interpreter
        self._def_context = definition_context
        self._statements = ast_statements
        self.attributes.add('run', BuiltinFunction(self.run))
        self.attributes.can_add = False

    def run(self, extra_vars_mapping=None):
        context = self._def_context.create_subcontext()
        if extra_vars_mapping is not None:
            for name, value in extra_vars_mapping.python_dict.items():
                context.namespace.add(name.python_string, value,
                                      is_const=False)

        for statement in self._statements:
            self._interp.execute(statement, context)


@BuiltinFunction
def print_(arg):
    assert isinstance(arg, String), "cannot print " + repr(arg)
    print(arg.python_string)
    return null


def add_builtins(namespace):
    namespace.add('null', null)
    namespace.add('print', print_)
    namespace.add('Mapping', BuiltinFunction(Mapping.from_pairs))
