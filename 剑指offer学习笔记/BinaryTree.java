import java.security.InvalidParameterException;
import java.util.ArrayDeque;
import java.util.Arrays;
import java.util.Queue;

public class BinaryTree {
    public BinaryTree left;
    public BinaryTree right;
    public int value;

    public BinaryTree(BinaryTree left, BinaryTree right, int value) {
        this.left = left;
        this.right = right;
        this.value = value;
    }

    public BinaryTree(int value) {
        this(null, null, value);
    }

    /**
     * 将给定的新左节点值插入到当前节点中：
     * 1. 如果当前节点没有左节点，新节点为当前节点的左节点。
     * 2. 如果当前节点有左节点，新节点为当前节点的左节点，原左节点作为新节点的左节点。
     *
     * @param currentNode 插入左节点的父节点，即当前节点
     * @param value 新左节点的值
     */
    public void insertLeft(BinaryTree currentNode, int value) {
        if (currentNode == null) {
            return;
        }

        BinaryTree newLeftNode = new BinaryTree(value);
        if (currentNode.left != null) {
            BinaryTree leftNode = currentNode.left;
            newLeftNode.left = leftNode;
        }
        currentNode.left = newLeftNode;
    }

    public void insertRight(BinaryTree currentNode, int value) {
        if (currentNode == null) {
            return;
        }

        BinaryTree newLeftNode = new BinaryTree(value);
        if (currentNode.right != null) {
            BinaryTree leftNode = currentNode.right;
            newLeftNode.right = leftNode;
        }
        currentNode.right = newLeftNode;
    }

    /**
     * 二叉搜索树插入新节点
     *
     * @param node 当前树，注意必须是二叉搜索树，新增节点后可能是二叉搜索树
     * @param value 新节点的值
     */
    public void insertNode(BinaryTree node, int value) {
        if (node == null) {
            return;
        }

        if (value <= Integer.valueOf(node.value) && node.left != null) {
            node.left.insertNode(node.left, value);
        } else if (value <= Integer.valueOf(node.value)) {
            node.left = new BinaryTree(value);
        } else if (value > Integer.valueOf(node.value) && node.right != null) {
            node.right.insertNode(node.right, value);
        } else {
            node.right = new BinaryTree(value);
        }
    }

    /**
     * 二叉搜索树查找节点是否存在
     *
     * @param node
     * @param value
     * @return
     */
    public boolean findNode(BinaryTree node, int value) {
        if (node == null) {
            return false;
        }
        if (value < Integer.valueOf(node.value) && node.left != null) {
            return node.left.findNode(node.left, value);
        }
        if (value > Integer.valueOf(node.value) && node.right != null) {
            return node.right.findNode(node.right, value);
        }
        return value == Integer.valueOf(node.value);
    }

    /**
     * 前序遍历
     *
     * @param node 二叉树的节点
     */
    public static void preOrder(BinaryTree node) {
        if (node != null) {
            System.out.println(node.value);
            if (node.left != null) {
                node.left.preOrder(node.left);
            }
            if (node.right != null) {
                node.right.preOrder(node.right);
            }
        }
    }

    /**
     * 中序遍历
     *
     * @param node 二叉树的节点
     */
    public static void inOrder(BinaryTree node) {
        if (node != null) {
            if (node.left != null) {
                node.left.inOrder(node.left);
            }
            System.out.println(node.value);
            if (node.right != null) {
                node.right.inOrder(node.right);
            }
        }
    }

    /**
     * 后序遍历
     *
     * @param node 二叉树的节点
     */
    public static void postOrder(BinaryTree node) {
        if (node != null) {
            if (node.left != null) {
                node.left.postOrder(node.left);
            }
            if (node.right != null) {
                node.right.postOrder(node.right);
            }
            System.out.println(node.value);
        }
    }

    /**
     * 广度优先搜索
     *
     * @param node 二叉树的节点
     */
    public static void bfsOrder(BinaryTree node) {
        if (node == null) {
            return;
        }

        Queue<BinaryTree> queue = new ArrayDeque<>();
        queue.add(node);

        while (!queue.isEmpty()) {
            BinaryTree currentNode = queue.poll();
            System.out.println(currentNode.value);
            if (currentNode.left != null) {
                queue.add(currentNode.left);
            }
            if (currentNode.right != null) {
                queue.add(currentNode.right);
            }
        }
    }

    /**
     * 二叉搜索树删除节点
     *
     * @param node 当前节点
     * @param value 指定被删除节点的值
     * @param parent 当前节点父节点
     * @return 成功返回true 失败返回false
     */
    public boolean removeNode(BinaryTree node, Integer value, BinaryTree parent) {
        if (node != null) {
            if (value < node.value && node.left != null) {
                return node.left.removeNode(node.left, value, node);
            } else if (value < node.value) {
                return false;
            } else if (value > node.value && node.right != null) {
                return node.right.removeNode(node.right, value, node);
            } else if (value > node.value) {
                return false;
            } else {
                if (node.left == null && node.right == null && node == parent.left) {
                    parent.left = null;
                    node.clearNode(node);
                } else if (node.left == null && node.right == null && node == parent.right) {
                    parent.right = null;
                    node.clearNode(node);
                } else if (node.left != null && node.right == null && node == parent.left) {
                    parent.left = node.left;
                    node.clearNode(node);
                } else if (node.left != null && node.right == null && node == parent.right) {
                    parent.right = node.left;
                    node.clearNode(node);
                } else if (node.right != null && node.left == null && node == parent.left) {
                    parent.left = node.right;
                    node.clearNode(node);
                } else if (node.right != null && node.left == null && node == parent.right) {
                    parent.right = node.right;
                    node.clearNode(node);
                } else {
                    node.value = node.right.findMinValue(node.right);
                    node.right.removeNode(node.right, node.right.value, node);
                }
                return true;
            }
        }
        return false;
    }

    /**
     * 查找二叉搜索树中的最小值坐在的节点
     *
     * @param node 二叉搜索树节点
     * @return 返回node树中，最小值所在的节点
     */
    public int findMinValue(BinaryTree node) {
        if (node != null) {
            if (node.left != null) {
                return node.left.findMinValue(node.left);
            } else {
                return value;
            }
        }
        return -1;
    }

    /**
     * 清空n节点
     *
     * @param node 需要被清空的节点
     */
    public void clearNode(BinaryTree node) {
        node.value = -1;
        node.left = null;
        node.right = null;
    }

    public static BinaryTree construct(int preOrder[], int inOrder[]) {
        if (preOrder == null || inOrder == null
            || preOrder.length != inOrder.length || preOrder.length <= 0) {
            return null;
        }

        return constructCore(preOrder, inOrder);
    }

    private static BinaryTree constructCore(int[] preOrder, int[] inOrder) {
        if (preOrder.length == 0 || inOrder.length == 0) {
            return null;
        }
        int rootValue = preOrder[0];
        BinaryTree root = new BinaryTree(rootValue);
        if (preOrder.length == 1) {
            if (inOrder[0] != rootValue) {
                throw new InvalidParameterException("preOrder and inOrder not match");
            }
            return root;
        }
        // 在中序中查找根节点
        int rootInorderIndex = 0;
        while (rootInorderIndex < inOrder.length && inOrder[rootInorderIndex] != rootValue) {
            rootInorderIndex++;
        }
        if (rootInorderIndex > 0) { // 构建左子树
            root.left = constructCore(Arrays.copyOfRange(preOrder, 1, rootInorderIndex + 1),
                Arrays.copyOf(inOrder, rootInorderIndex));
        }
        if (rootInorderIndex < preOrder.length) { // 构建右子树
            root.right = constructCore(Arrays.copyOfRange(preOrder, rootInorderIndex + 1, preOrder.length),
                Arrays.copyOfRange(inOrder, rootInorderIndex + 1, inOrder.length));
        }
        return root;
    }
}
