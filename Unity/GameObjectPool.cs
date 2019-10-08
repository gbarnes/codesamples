using ImmersivePixels.Core.Interfaces;
using ImmersivePixels.Debug;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using UnityEngine;
using UnityEngine.Assertions;

namespace ImmersivePixels.Core.Pooling
{
    /// <summary>
    /// A pool that loads gameobjects via a specified loading policy. 
    /// </summary>
    /// <typeparam name="P"></typeparam>
    public class GameObjectPool<P> : ObjectPool<GameObject, P> where P : IObjectLoadPolicy<GameObject>, new()
    {
        #region Private Members
        private GameObject _parent;
        private GameObject _original;
        #endregion

        #region Constructor
        /// <summary>
        /// Creates a new instance!
        /// </summary>
        /// <param name="parent">The parent object which funcrtions as a parent transform</param>
        /// <param name="sizeOfPool">The size of the pool</param>
		/// <param name="name="instantiatePolicy">Should the specified object policy be instantiated automatically</param>
        /// <param name="grow">Should the pool grow automatically?</param>
		public GameObjectPool(GameObject parent, int sizeOfPool, bool instantiatePolicy = true, bool grow = false)
            : base(sizeOfPool, grow)
        {
            this._parent = parent;
            this._original = null;
        }
        #endregion

        #region Public Methods
        /// <summary>
        /// Takes care of allocating the pool objects at a specific path.
        /// </summary>
        /// <param name="objectPath">The path of the object.</param>
        /// <returns>True or false depending on the result of the loading process</returns>
        public override bool Allocate(string objectPath = "")
        {
            if (!this._isInitialized)
                return false;

            Assert.IsNotNull(this._objectLoadPolicy, "No Object Load Policy specified! In order to run the pool you need one!");
            Assert.IsTrue(this._objectLoadPolicy.IsCreated(), "Object Load Policy has not been created please call Initialize on Object Pool!");
            Assert.IsNotNull(this._parent, "The parent object is null! You need to specify a parent object at construction time!");

            UnityEngine.Object original = this._objectLoadPolicy.Load(objectPath);
            if (original == null)
            {
                DLog.Error("Couldn't find original allocation object for pool ( " + objectPath + ").", DLogCategory.Loading);
                return false;
            }

            this._original = (GameObject)GameObject.Instantiate(original);
            this._original.transform.parent = this._parent.transform;
            this._original.SetActive(false);
            this._original.name = "GameObject_Reference";


            try
            {
                for (int i = 0; i < this._sizeOfPool; i++)
                {
                    GameObject obj = UnityEngine.Object.Instantiate(_original);
                    if (obj == null)
                        return false;

                    obj.name = "GameObject";
                    obj.transform.parent = this._parent.transform;
                    obj.SetActive(false);
                    this._pool.Enqueue(obj);
                }
            }
            catch (Exception ex)
            {
                DLog.Error(ex, DLogCategory.Loading);
                return false;
            }

            return true;
        }

        /// <summary>
        /// Allocates a single object.
        /// </summary>
        /// <returns>The game object or null</returns>
        public override GameObject AllocateSingle()
        {
            if (!this._isInitialized)
                return null;

            Assert.IsNotNull(this._original, "Please call allocate beforehand trying to allocate a new single object!");

            try
            {
                GameObject obj = UnityEngine.Object.Instantiate(_original);
                if (obj == null)
                    return null;

                obj.transform.parent = this._parent.transform;
                obj.SetActive(false);
                return obj;
            }
            catch (Exception ex)
            {
                DLog.Error(ex, DLogCategory.Loading);
                return null;
            }
        }

        /// <summary>
        /// Destroys all game objects in the pool. Be aware that 
        /// this won't destroy dangling references of objects that
        /// are outside the control of this pool!
        /// </summary>
        public override void Unload()
        {
            while (this._pool.Count > 0)
                GameObject.DestroyImmediate(this._pool.Dequeue());

            GameObject.DestroyImmediate(this._original);
            GameObject.DestroyImmediate(this._parent);
        }

        /// <summary>
        /// Returns a valid gameobject from the pool.
        /// </summary>
        /// <returns>The gameobject or null</returns>
        public override GameObject Get()
        {
            if (!this._isInitialized)
                return null;
            if (this._pool.Count == 0)
            {
                if (this._grow)
                {
                    this._sizeOfPool += 1;
                    return AllocateSingle();
                }
                return null;
            }
            return this._pool.Dequeue();
        }

        /// <summary>
        /// Returns a gameobject back to the pool. This will fail if 
		/// the pool isn't initialized.
        /// </summary>
        /// <param name="obj">The gameobject to return.</param>
        override public void Return(GameObject obj)
        {
            if (!this._isInitialized)
                return;

            if (!this._pool.Contains(obj))
                this._pool.Enqueue(obj);

            obj.transform.position = Vector3.zero;
            obj.transform.rotation = this._original.transform.rotation;
            obj.SetActive(false);

            if (this._parent != null)
            {
                obj.transform.SetParent(this._parent.transform);
                obj.name = "GameObject";
            }
            else
            {
                DLog.Error("Parent is null when adding obj: " + obj.name, DLogCategory.Loading);
            }
        }
        #endregion
    }
}
