using ImmersivePixels.Core.Interfaces;
using ImmersivePixels.Debug;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace ImmersivePixels.Core.Pooling
{
    /// <summary>
    /// A generic pool implementation. For a gameobject pool this needs to be extended. 
    /// </summary>
    /// <typeparam name="T">The object</typeparam>
    public abstract class ObjectPool<T, P> where T : class where P : IObjectLoadPolicy<T>, new()
    {
        #region Protected Members
        protected bool _grow = false;
        #endregion

        #region Private Members
        protected IObjectLoadPolicy<T> _objectLoadPolicy;
        protected Queue<T> _pool;
        protected int _sizeOfPool;
        protected bool _isInitialized;
        #endregion

        #region Constructor
        /// <summary>
        /// Creates a new pool of objects with a given size.
        /// </summary>
        /// <param name="sizeOfPool">The number of objects within the pool.</param>
		public ObjectPool(int sizeOfPool, bool grow = false, bool instantiatePolicy = true)
        {
            this._sizeOfPool = sizeOfPool;
            this._pool = new Queue<T>();
            this._grow = grow;
            this._isInitialized = false;

            if (instantiatePolicy)
                this._objectLoadPolicy = new P();
        }
        #endregion

        #region Public Mehtods
        /// <summary>
        /// Initializes the pool and it's load policy!
        /// </summary>
		public virtual void Initialize(IObjectLoadPolicy<T> policy = null, params object[] args)
        {
            if (policy != null)
                this._objectLoadPolicy = policy;

            if (this._objectLoadPolicy != null)
            {
                if (!this._objectLoadPolicy.IsCreated())
                    this._objectLoadPolicy.Create(args);
            }

            this._isInitialized = true;
        }

        /// <summary>
        /// Unloads all allocated resources!
        /// </summary>
        public virtual void Unload()
        {
            this._pool.Clear();
        }

        /// <summary>
        /// Allocates a single element! 
        /// (ATTENTION THIS WILL RETURN AN OBJECT THAT ISN'T MANAGED BY THE POOL)
        /// </summary>
        /// <returns></returns>
        public virtual T AllocateSingle()
        {
            if (!this._isInitialized)
                return null;

            if (this._objectLoadPolicy == null)
            {
                DLog.Error("Did you forget to set your object policy?!", DLogCategory.Loading);
                return null;
            }

            try
            {
                T obj = this._objectLoadPolicy.Load();
                return obj;
            }
            catch (Exception ex)
            {
                DLog.Error(ex, DLogCategory.Loading);
                return null;
            }
        }

        /// <summary>
        /// Gets a value from the pool if there is any left. If not it will return null!
        /// </summary>
        /// <returns>Null or the instance of the object</returns>
        public virtual T Get()
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
                return (T)null;
            }

            return this._pool.Dequeue();
        }

        /// <summary>
        /// Returns the object to the pool so its available again to get pooled!
        /// </summary>
        /// <param name="obj">The object</param>
        virtual public void Return(T obj)
        {
            if (!this._isInitialized)
                return;
            if (!this._pool.Contains(obj))
                this._pool.Enqueue(obj);
        }

        /// <summary>
        /// Returns the current number of objects in the pool!
        /// </summary>
        /// <returns>The number of objects</returns>
        public int GetCount()
        {
            if (!this._isInitialized)
                return 0;

            return this._pool.Count;
        }

        /// <summary>
        /// Returns the current size of the pool!
        /// </summary>
        /// <returns>The size of the pool</returns>
        public int GetSizeOfPool()
        {
            return this._sizeOfPool;
        }

        /// <summary>
        /// Sets the size of the pool 
        /// </summary>
        /// <param name="size"></param>
        private void SetSizeOfPool(int size)
        {
            this._sizeOfPool = size;
        }

        /// <summary>
        /// Allocates the predfined number of objects within the pool
        /// </summary>
        /// <returns></returns>
		public virtual bool Allocate(string objectPath = "")
        {
            if (!this._isInitialized)
                return false;

            if (this._objectLoadPolicy == null)
            {
                DLog.Error("Did you forget to set your object policy?!", DLogCategory.Loading);
                return false;
            }

            try
            {
                for (int i = 0; i < this._sizeOfPool; i++)
                {
                    T obj = this._objectLoadPolicy.Load();
                    if (obj == null)
                        return false;

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
        #endregion
    }
}
